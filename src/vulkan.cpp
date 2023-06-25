#include "vulkan.hpp"

#include "TaskScheduler.h"

#include "imgui/imgui.h"

#include "vulkan/vulkan.h"

#include "depth_helper.hpp"
#include "swapchain.hpp"

#include <deque>
#include <chrono>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <mutex>

#include "device.hpp"
#include "fence.hpp"
#include "image.hpp"
#include "renderpass.hpp"
#include "framebuffer.hpp"
#include "fence.hpp"
#include "buffer.hpp"
#include "command_buffer.hpp"
#include "descriptor_sets.hpp"
#include "pipeline.hpp"
#include "load_ktx.hpp"
#include "compute/particles.hpp"
#include "compute/sand.hpp"
#include "render/draw_particles.hpp"
#include "render/draw_triangle.hpp"
#include "render/draw_ui.hpp"
#include "render/draw_fullscreen.hpp"

#include "graph/fake_node.hpp"

#include "ui/main_ui.hpp"

#include "glm/glm.hpp"

#include "host_scheduler.hpp"

#include <chrono>
#include <random>

#include "inputs/sane/sane_service.hpp"

namespace vkd {

    namespace {
        bool _enableValidation = false;

	    std::shared_ptr<MainUI> _ui = nullptr;

        std::shared_ptr<Instance> _instance;
        std::shared_ptr<Device> _device;
        VkFormat _depth_format, _colour_format;
        std::vector<VkCommandBuffer> _command_buffers;

        std::unique_ptr<swapchain> _swapchain = nullptr;
        std::shared_ptr<Surface> _surface = nullptr;
        std::unique_ptr<Image> _depth_image = nullptr;
        std::unique_ptr<Image> _colour_image = nullptr;

        std::vector<std::unique_ptr<Framebuffer>> _framebuffers;

        VkSemaphore _present_complete;
        VkSemaphore _render_complete;
        std::vector<FencePtr> _command_buffer_complete;

        std::shared_ptr<Renderpass> _renderpass = nullptr;
        std::shared_ptr<PipelineCache> _pipeline_cache = nullptr;

        std::shared_ptr<DrawUI> _draw_ui = nullptr;

        uint32_t _width = 0;
        uint32_t _height = 0;
        std::unique_ptr<HostScheduler> _task_scheduler = nullptr;

    }

    HostScheduler& ts() { return *_task_scheduler; }

    void shutdown() {

        sane::Service::Shutdown();

        vkDeviceWaitIdle(_device->logical_device());

        vkDestroySemaphore(_device->logical_device(), _present_complete, nullptr);
        vkDestroySemaphore(_device->logical_device(), _render_complete, nullptr);

	    _ui = nullptr;

        _draw_ui = nullptr;

        _task_scheduler->cleanup();

        _pipeline_cache = nullptr;
        _renderpass = nullptr;
        _framebuffers.clear();
        
        //std::vector<VkCommandBuffer> _command_buffers;
        //VkFence _fence;
        //std::vector<VkFence> _command_buffer_complete;

        _command_buffer_complete.clear();

        _swapchain = nullptr;
        _surface = nullptr;
        _depth_image = nullptr;
        _colour_image = nullptr;

        //VkSemaphore _present_complete;
        //VkSemaphore _render_complete;

        _device = nullptr;
        _instance = nullptr;

        _task_scheduler = nullptr;
    }

	Device& device() { return *_device; }
	DrawUI& get_ui() { return *_draw_ui; }

    std::shared_ptr<Instance> createInstance(bool enableValidation) {
        auto instance = std::make_shared<Instance>();
        instance->init(enableValidation);
        return instance;
    }

    void init(SDL_Window * window, SDL_Renderer * renderer) {
        
        _task_scheduler = std::make_unique<HostScheduler>();
        _task_scheduler->init();

        _instance = createInstance(true);
        _device = std::make_shared<Device>(_instance);
        _device->create(_instance->get_physical_device());

        get_depth_format(_instance->get_physical_device(), &_depth_format);
        
        _surface = std::make_shared<Surface>(_instance);
        _surface->init(window, renderer, _device);
        _colour_format = _surface->colour_format();

        _swapchain = std::make_unique<swapchain>(_instance, _device, _surface);

        _width = 1280;
        _height = 720;

        _swapchain->create(_width, _height, true);
        _command_buffers.resize(_swapchain->count());
        for (auto&& command_buffer : _command_buffers) {
            command_buffer = create_command_buffer(_device->logical_device(), _device->command_pool());
        }

        _depth_image = std::make_unique<Image>(_device);
        _depth_image->create_image(_depth_format, {_width, _height}, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        _depth_image->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        // Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
        VkImageAspectFlags depth_view_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (_depth_format == VK_FORMAT_D16_UNORM_S8_UINT 
            || _depth_format == VK_FORMAT_D24_UNORM_S8_UINT 
            || _depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT) { // this is not good code wtf
            depth_view_flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        _depth_image->create_view(depth_view_flags);

        _renderpass = std::make_shared<Renderpass>(_device);
        _renderpass->create(_colour_format, _depth_format);

        int count = _swapchain->count();
        for (int i = 0; i < count; ++i) {
            auto framebuffer = std::make_unique<Framebuffer>(_device, _renderpass);
            framebuffer->create(_swapchain->image(i), *_depth_image, _width, _height);
            _framebuffers.emplace_back(std::move(framebuffer));
        }

        _present_complete = create_semaphore(_device->logical_device());
        _render_complete = create_semaphore(_device->logical_device());
        _command_buffer_complete.resize(_swapchain->count());
        for (auto&& fence : _command_buffer_complete) {
            fence = Fence::create(_device, true);
        }

        _pipeline_cache = std::make_shared<PipelineCache>(_device);
        _pipeline_cache->create();
        
        _draw_ui = std::make_shared<DrawUI>(_swapchain->count());
        engine_node_init(_draw_ui, "__ui");
        _draw_ui->init();

        _ui = std::make_unique<MainUI>();
        _ui->set_device(_device);

        sane::Service::Get().init();
    }

    void engine_node_init(const std::shared_ptr<EngineNode>& node, const std::string& param_hash_name) {
        node->set_param_hash_name(param_hash_name);
        node->set_device(_device);
        node->set_pipeline_cache(_pipeline_cache);
        node->set_renderpass(_renderpass);

        node->post_setup();
    }

    std::shared_ptr<vkd::EngineNode> make(const std::string& node_type, const std::string& param_hash_name) {
        std::shared_ptr<EngineNode> ret = nullptr;


        auto&& nodemap = vkd::EngineNode::node_type_map();
        auto search = nodemap.find(node_type);
        if (search != nodemap.end()) {
            ret = search->second.clone->clone();
        } else {
            return nullptr;
        } /*else if (name == "read") {
            
        } else if (name == "write") {
            
        } else if (name == "triangles") {
            ret = std::make_shared<vkd::DrawTriangle>(_width/(float)_height);
        } else if (name == "particles") {
            ret = std::make_shared<vkd::Particles>();
        } else if (name == "draw_particles") {
            ret = std::make_shared<vkd::DrawParticles>();
        } else if (name == "history") {
            
        } else if (name == "sand") {
            ret = std::make_shared<vkd::Sand>(_width, _height);
        } else if (name == "draw") {
            ret = std::make_shared<vkd::DrawFullscreen>();
        }*/

        if (ret) {
            engine_node_init(ret, param_hash_name);
            //ret->init();
        }
        return ret;
    }

    void build_command_buffers(VkCommandBuffer buf, int index) {
        begin_command_buffer(buf);
        _renderpass->begin(buf, _framebuffers[index]->framebuffer(), _width, _height);

        _ui->commands(buf, _width, _height);

        _draw_ui->commands(buf, _width, _height);

        _renderpass->end(buf);
        end_command_buffer(buf);
    }

    void submit_buffer(VkQueue queue, VkCommandBuffer buf, Fence * fence) {
		// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
		std::vector<VkPipelineStageFlags> wait_stage_masks = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		
        std::vector<VkSemaphore> wait_semaphores = {_present_complete};
        std::vector<VkSemaphore> present_semaphores = {_render_complete};

        auto& stream = _ui->stream();
        wait_semaphores.push_back(stream.semaphore().get());
        wait_stage_masks.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        present_semaphores.push_back(stream.semaphore().get());
        
        auto timeline_value = stream.semaphore().increment();
		std::vector<uint64_t> wait_values = {0, timeline_value - 1};
		std::vector<uint64_t> signal_values = {0, timeline_value};

		VkTimelineSemaphoreSubmitInfo timeline_info;
		timeline_info.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		timeline_info.pNext = NULL;
		timeline_info.waitSemaphoreValueCount = wait_values.size();
		timeline_info.pWaitSemaphoreValues = wait_values.data();
		timeline_info.signalSemaphoreValueCount = signal_values.size();
		timeline_info.pSignalSemaphoreValues = signal_values.data();

		VkSubmitInfo submit_info = {};
		submit_info.pNext = &timeline_info;
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.pWaitDstStageMask = wait_stage_masks.data();               // Pointer to the list of pipeline stages that the semaphore waits will occur at
		submit_info.pWaitSemaphores = wait_semaphores.data();      // Semaphore(s) to wait upon before the submitted command buffer starts executing
		submit_info.waitSemaphoreCount = wait_semaphores.size();                           // One wait semaphore
		submit_info.pSignalSemaphores = present_semaphores.data();     // Semaphore(s) to be signaled when command buffers have completed
		submit_info.signalSemaphoreCount = present_semaphores.size();                         // One signal semaphore
		submit_info.pCommandBuffers = &buf; // Command buffers(s) to execute in this batch (submission)
		submit_info.commandBufferCount = 1;                           // One command buffer

		// Submit to the graphics queue passing a wait fence
        {
            if (fence) { fence->pre_submit(); }
            std::scoped_lock lock(_device->queue_mutex());
            double wait_time = 0.5;
            VkResult res = VK_SUCCESS;
            for (int i = 0; i < 20; ++i) {
                res = vkQueueSubmit(queue, 1, &submit_info, fence ? fence->get() : nullptr);
                VK_CHECK_RESULT_TIMEOUT(res);
                if (res == VK_SUCCESS) {
                    if (fence) { fence->mark_submit(); }
                    break;
                }
                console << "Slow: Timeout waiting on Queue Submit (" << wait_time * (i + 1) << ")." << std::endl;
            }
            if (res == VK_TIMEOUT) {
                console << "Slow: Queue wait failed after 10s, issues likely inbound." << std::endl;
            }
        }
    }

    namespace {
        enki::TaskSet * _callback_task = nullptr;
    }

    void render_callback(uint32_t current_buffer, Stream& stream) {

        if (_callback_task) {
            _task_scheduler->wait(_callback_task);
        }
        auto task = std::make_unique<enki::TaskSet>(1, [current_buffer, &stream](enki::TaskSetPartition range, uint32_t threadnum) {
            try {
                stream.flush();
                _draw_ui->flush();
            } catch (...) {

            }
        });
        _callback_task = _task_scheduler->add(std::move(task));
    }

	void draw() {

        _ui->update();

        uint32_t current_buffer = _swapchain->next_image(_present_complete);

		// Use a fence to wait until the command buffer has finished execution before using it again
        _command_buffer_complete[current_buffer]->wait();
        _command_buffer_complete[current_buffer]->reset();

        if (_draw_ui->update(ExecutionType::UI)) {
            build_command_buffers(_command_buffers[current_buffer], current_buffer);
        }

        _ui->execute();
        
        submit_buffer(_device->queue(), _command_buffers[current_buffer], _command_buffer_complete[current_buffer].get());
        
        render_callback(current_buffer, _ui->stream());

		// Present the current buffer to the swap chain
		// Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
		// This ensures that the image is not presented to the windowing system until all commands have been submitted
		_swapchain->present(_render_complete, current_buffer);

        _task_scheduler->cleanup();
	}

    void ui(bool& quit) {
        ImGui::SetNextWindowPos(ImVec2(2, 20), ImGuiCond_Always );
        ImGui::SetNextWindowSize(ImVec2(100, 20), ImGuiCond_Always );
        ImGui::Begin("fps", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

        static std::deque<float> frame_durations;
        static std::chrono::high_resolution_clock::time_point last_frame;

        auto tp = std::chrono::high_resolution_clock::now();
        auto gap = tp - last_frame;
        
        //double dist = std::chrono::duration_cast<double>(gap).count();
        frame_durations.push_back((double)1000000000 / gap.count());
        last_frame = tp;

        if (frame_durations.size() > 10) {
            frame_durations.pop_front();
        }

        double avg = 0.0;
        for (auto&& dur : frame_durations) {
            avg += (double)dur;
        }

        avg = avg / (double)frame_durations.size();

        ImGui::Text("%3.2f fps", avg);

        ImGui::End();

        if (_draw_ui->get_ui_visible() == DrawUI::UiVisible::FPS) {
            return;
        }   

        if (_ui->vulkan_window_open()) {
            ImGui::SetNextWindowCollapsed(false, ImGuiCond_Once);
            ImGui::SetNextWindowPos(ImVec2(25, 45), ImGuiCond_Once );
            ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_Once );
            ImGui::Begin("Vulkan", &_ui->vulkan_window_open());

            if (ImGui::TreeNode("Supported Extensions")) {
                std::stringstream strm;
                for (auto&& ext : _instance->supported_instance_extensions()) {
                    strm << ext << "\n";
                }
                ImGui::Text("%s", strm.str().c_str());
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Enabled Extensions")) {
                std::stringstream strm;
                for (auto&& ext : _instance->enabled_instance_extensions()) {
                    strm << ext << "\n";
                }
                ImGui::Text("%s", strm.str().c_str());
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Instance Layers")) {
                std::stringstream strm;
                for (auto&& layer : _instance->instance_layer_properties()) {
                    strm << layer.layerName << ": " << layer.description << "\n";
                }
                ImGui::Text("%s", strm.str().c_str());
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Physical Devices")) {
                int i = 0;
                for (auto&& device : _instance->physical_device_props()) {
                    std::stringstream strm;
                    auto&& feats = _instance->physical_device_feats()[i];
                    auto&& memProps = _instance->physical_device_mem_props()[i];
                    strm << device.deviceName 
                    << " - Type: " << physical_device_to_string(device.deviceType)
                    << " - API: " << (device.apiVersion >> 22) << "." << ((device.apiVersion >> 12) & 0x3ff) << "." << (device.apiVersion & 0xfff) << "\n";

                    ImGui::Text("%s", strm.str().c_str());

                    if (ImGui::TreeNode("Feats")) {
                        ImGui::Text("%s", physical_device_features_string(feats).c_str());
                        ImGui::TreePop();
                    }

                    std::stringstream strm2;
                    strm2 << "Heaps: " << memProps.memoryHeapCount << " Types: " << memProps.memoryTypeCount << "\n";
                    for (int j = 0; j < memProps.memoryHeapCount; ++j) {
                        strm2 << "Heap " << j << " - size: " << memProps.memoryHeaps[j].size 
                            << "\n flags: \n" << memory_heap_flags_to_string(memProps.memoryHeaps[j].flags);
                    }

                    for (int j = 0; j < memProps.memoryTypeCount; ++j) {
                        strm2 << "Type " << j 
                            << " - heap index: " << memProps.memoryTypes[j].heapIndex 
                            << "\n flags: \n" << memory_property_flags_to_string(memProps.memoryTypes[j].propertyFlags);
                    }
                    ImGui::Text("%s", strm2.str().c_str());
                    
                    i++;
                }

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Logical Device Props")) {
                
                int i = 0;
                std::stringstream strm;

                strm << "Device " << i << "\n\t Queue count: " 
                    << _device->queue_family_props()[i].queueCount
                    << "\n\t minImageTransferGranularity: " << _device->queue_family_props()[i].minImageTransferGranularity.width 
                    << "/" << _device->queue_family_props()[i].minImageTransferGranularity.height << "/" << _device->queue_family_props()[i].minImageTransferGranularity.depth
                    << "\n\t timestampValidBits: " << _device->queue_family_props()[i].timestampValidBits
                    << "\n\t queueFlags: \n" << queue_flags_to_string(_device->queue_family_props()[i].queueFlags) << "\n";


                ImGui::Text("%s", strm.str().c_str());
                if (ImGui::TreeNode("Extensions")) {
                    std::stringstream strm3;
                    for (auto&& ext : _device->device_extension_props()) {
                        strm3 << ext.extensionName << ": " << ext.specVersion << "\n";
                    }

                    ImGui::Text("%s", strm3.str().c_str());
                    ImGui::TreePop();
                }

                std::stringstream strm2;
                strm2 << "Created device: " << (_device->logical_device() ?  "true" : "false") << "\n";
                strm2 << "Created queue: " << (_device->queue() ?  "true" : "false") << "\n";
                strm2 << "Enabled extensions:" << "\n";
                for (auto&& ext : _device->device_extensions_enabled()) {
                    strm2 << "\t" << ext << "\n";
                }
                
                
                strm2 << "Colour format: " << format_to_string(_colour_format) << "\n";
                strm2 << "Depth format: " << format_to_string(_depth_format) << "\n";

                ImGui::Text("%s", strm2.str().c_str());
                i++;


                if (ImGui::TreeNode("Feats")) {
                    ImGui::Text("%s", physical_device_features_string(_device->features()).c_str());
                    ImGui::Text("%s", physical_device_8bit_features_string(_device->ext_8bit_features()).c_str());
                    ImGui::Text("%s", physical_device_16bit_features_string(_device->ext_16bit_features()).c_str());
                    ImGui::TreePop();
                }


                ImGui::TreePop();
            }

            _swapchain->ui();

            ImGui::End();
        }

        _ui->draw();
        quit = quit || _ui->has_quit();
    }
}