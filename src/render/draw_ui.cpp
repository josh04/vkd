#include "draw_ui.hpp"

#include "pipeline.hpp"
#include "imgui/imgui.h"

#include "descriptor_sets.hpp"
#include "buffer.hpp"
#include "pipeline.hpp"
#include "device.hpp"
#include "image.hpp"
#include "command_buffer.hpp"
#include "shader.hpp"
#include "vertex_input.hpp"
#include "sampler.hpp"
#include "renderpass.hpp"
#include "viewport_and_scissor.hpp"

#include "imgui/imgui_impl_vulkan.h"

namespace vkd {
    class ImGuiPipelineLayout : public vkd::PipelineLayout {
    public:
        using vkd::PipelineLayout::PipelineLayout;

        void constant(VkPipelineLayoutCreateInfo& info) override {

			VkPushConstantRange push_constant_range = {};
			push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			push_constant_range.offset = 0;
			push_constant_range.size = sizeof(DrawUI::PushConstBlock);

            _push_constant_ranges.push_back(push_constant_range);
        }
    };

    class ImGuiPipeline : public vkd::GraphicsPipeline {
        using vkd::GraphicsPipeline::GraphicsPipeline;

        void _blend_attachments() override {
            VkPipelineColorBlendAttachmentState blend_attachment_state = {};
            blend_attachment_state.blendEnable = VK_TRUE;
            blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
            blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

            _blend_attachment_states.push_back(blend_attachment_state);
        }
    };

    namespace {
        ImGui_ImplVulkan_InitInfo imgui_init;
    }

    DrawUI::~DrawUI() {
        ImGui_ImplVulkan_DestroyFontUploadObjects();
        ImGui_ImplVulkan_Shutdown();
    }
/*
// Initialization data, for ImGui_ImplVulkan_Init()
// [Please zero-clear before use!]
struct ImGui_ImplVulkan_InitInfo
{
    VkInstance                      Instance;
    VkPhysicalDevice                PhysicalDevice;
    VkDevice                        Device;
    uint32_t                        QueueFamily;
    VkQueue                         Queue;
    VkPipelineCache                 PipelineCache;
    VkDescriptorPool                DescriptorPool;
    uint32_t                        Subpass;
    uint32_t                        MinImageCount;          // >= 2
    uint32_t                        ImageCount;             // >= MinImageCount
    VkSampleCountFlagBits           MSAASamples;            // >= VK_SAMPLE_COUNT_1_BIT
    const VkAllocationCallbacks*    Allocator;
    void                            (*CheckVkResultFn)(VkResult err);
};

*/
    void DrawUI::init() {
        memset(&imgui_init, 0 , sizeof(ImGui_ImplVulkan_InitInfo));
        imgui_init.Instance = _device->instance()->get();
        imgui_init.PhysicalDevice = _device->physical_device();
        imgui_init.Device = _device->logical_device();
        imgui_init.QueueFamily = _device->queue_index();
        imgui_init.Queue = _device->queue();
        imgui_init.PipelineCache = _pipeline_cache->get();
        
        {
            VkDescriptorPoolSize pool_sizes[] =
            {
                { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };
            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
            pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
            pool_info.pPoolSizes = pool_sizes;
            VkResult err = vkCreateDescriptorPool(_device->logical_device(), &pool_info, nullptr, &imgui_init.DescriptorPool);
            VK_CHECK_RESULT(err);
        }

        //_desc_pool = std::make_shared<vkd::DescriptorPool>(_device);
        //_desc_pool->add_combined_image_sampler(2);
        //_desc_pool->create(1); // one for gfx, one for compute

        //imgui_init.DescriptorPool = _desc_pool->get();
        imgui_init.Subpass = 0;
        imgui_init.MinImageCount = _swapchain_count;
        imgui_init.ImageCount = _swapchain_count;
        imgui_init.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        imgui_init.Allocator = nullptr;
        imgui_init.CheckVkResultFn = [](VkResult r){ VK_CHECK_RESULT(r); };


        ImGui_ImplVulkan_Init(&imgui_init, _renderpass->get());

        auto buf = vkd::begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());
        ImGui_ImplVulkan_CreateFontsTexture(buf);
        vkd::flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);


        /*
        create_font();

        _sampler = vkd::create_sampler(_device->logical_device());

        _desc_pool = std::make_shared<vkd::DescriptorPool>(_device);
        _desc_pool->add_combined_image_sampler(2);
        _desc_pool->create(1); // one for gfx, one for compute

        _desc_set_layout = std::make_shared<vkd::DescriptorLayout>(_device);
        _desc_set_layout->add(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        _desc_set_layout->create();

        _desc_set = std::make_shared<vkd::DescriptorSet>(_device, _desc_set_layout, _desc_pool);
        _desc_set->add_image(*_font_images[0], _sampler);
        _desc_set->create();

        auto pl = std::make_shared<ImGuiPipelineLayout>(_device);
        pl->create(_desc_set_layout->get());

        _pipeline = std::make_shared<ImGuiPipeline>(_device, _pipeline_cache, _renderpass);

        auto shader_stages = std::make_unique<vkd::Shader>(_device);
        shader_stages->add(VK_SHADER_STAGE_VERTEX_BIT, "shaders/base/uioverlay.vert.spv", "main");
        shader_stages->add(VK_SHADER_STAGE_FRAGMENT_BIT, "shaders/base/uioverlay.frag.spv", "main");
        
        auto vertex_input = std::make_unique<vkd::VertexInput>();
        vertex_input->add_binding(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX);
        vertex_input->add_attribute(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos));
        vertex_input->add_attribute(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv));
        vertex_input->add_attribute(0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col));

        _pipeline->create(pl, std::move(shader_stages), std::move(vertex_input));
        */
    }

    bool DrawUI::update() {
        
       // bool update_buffers = false;
/*
        ImDrawData* imDrawData = ImGui::GetDrawData();
        if (!imDrawData) { return false; }

        // Note: Alignment is done inside buffer creation
        VkDeviceSize vbuffer_size = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
        VkDeviceSize ibuffer_size = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

        // Update buffers only if vertex or index count has been changed compared to current buffer size
        if ((vbuffer_size == 0) || (ibuffer_size == 0)) {
            return false;
        }

        // Vertex buffer
        if (_vertex_buf == nullptr || (vbuffer_size != _vertex_buf->requested_size())) {
            if (_vertex_buf) {
                auto buf = vkd::begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());
                _vertex_buf->barrier(buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
                flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);
            }
            
            _vertex_buf = std::make_shared<vkd::VertexBuffer>(_device);
            _vertex_buf->create(vbuffer_size);
            update_buffers = true;
        }

        // Index buffer
        if (_index_buf == nullptr || (ibuffer_size > _index_buf->requested_size())) {
            if (_index_buf) {
                auto buf = vkd::begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());
                _index_buf->barrier(buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
                flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);
            }
            _index_buf = std::make_shared<vkd::IndexBuffer>(_device);
            _index_buf->create(ibuffer_size);
            update_buffers = true;
        }

        std::vector<std::pair<void *, size_t>> vdata;
        std::vector<std::pair<void *, size_t>> idata;

        for (int n = 0; n < imDrawData->CmdListsCount; n++) {
            const ImDrawList* cmd_list = imDrawData->CmdLists[n];
            vdata.push_back({cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert)});
            idata.push_back({cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx)});
        }

        _vertex_buf->stage(vdata);
        _index_buf->stage(idata);

        update_buffers = true;
        return update_buffers;
        */

       return true;
    }

    /*

IMGUI_IMPL_API bool     ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo* info, VkRenderPass render_pass);
IMGUI_IMPL_API void     ImGui_ImplVulkan_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplVulkan_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplVulkan_RenderDrawData(ImDrawData* draw_data, VkCommandBuffer command_buffer, VkPipeline pipeline = VK_NULL_HANDLE);
IMGUI_IMPL_API bool     ImGui_ImplVulkan_CreateFontsTexture(VkCommandBuffer command_buffer);
IMGUI_IMPL_API void     ImGui_ImplVulkan_DestroyFontUploadObjects();
IMGUI_IMPL_API void     ImGui_ImplVulkan_SetMinImageCount(uint32_t min_image_count); // To override MinImageCount after initialization (e.g. if swap chain is recreated)

    */
    void DrawUI::commands(VkCommandBuffer buf, uint32_t width, uint32_t height) {
        if (_ui_visible == UiVisible::Off) {
            return;
        }

        ImDrawData* imDrawData = ImGui::GetDrawData();
        if (!imDrawData) { return; }
        ImGui_ImplVulkan_RenderDrawData(imDrawData, buf, VK_NULL_HANDLE);


/*
        viewport_and_scissor(buf, width, height, width, height);

        ImDrawData* draw_data = ImGui::GetDrawData();

        int32_t vertex_offset = 0;
        int32_t index_offset = 0;

        if ((!draw_data) || (draw_data->CmdListsCount == 0)) {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();

        _pipeline->bind(buf, _desc_set->get());

        _const_block.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
        _const_block.translate = glm::vec2(-1.0f);
        vkCmdPushConstants(buf, _pipeline->layout()->get(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &_const_block);

        VkDeviceSize offset = 0;
        auto vbuf = _vertex_buf->buffer();
        vkCmdBindVertexBuffers(buf, 0, 1, &vbuf, &offset);
        vkCmdBindIndexBuffer(buf, _index_buf->buffer(), 0, VK_INDEX_TYPE_UINT16);

        for (int32_t i = 0; i < draw_data->CmdListsCount; i++) {
            const ImDrawList* cmd_list = draw_data->CmdLists[i];
            for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];

                VkRect2D scissor_rect;

                scissor_rect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
                scissor_rect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
                scissor_rect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                scissor_rect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);

                vkCmdSetScissor(buf, 0, 1, &scissor_rect);

                vkCmdDrawIndexed(buf, pcmd->ElemCount, 1, index_offset, vertex_offset, 0);

                index_offset += pcmd->ElemCount;
            }
            vertex_offset += cmd_list->VtxBuffer.Size;
        }
        */
    }

    void DrawUI::execute(VkSemaphore wait_semaphore, VkFence fence) {

    }

    void DrawUI::create_font() {
        /*
        ImGuiIO& io = ImGui::GetIO();

        unsigned char* pixels = nullptr;
        int width = 0, height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        auto staging = std::make_shared<vkd::StagingImage>(_device);
        staging->create_image(VK_FORMAT_R8G8B8A8_UNORM, width, height);

        auto ptr = staging->map();
        memcpy(ptr, pixels, width * height * 4);
        staging->unmap();

        auto fontI = std::make_shared<vkd::Image>(_device);
        fontI->create_image(VK_FORMAT_R8G8B8A8_UNORM, width, height, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        fontI->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        fontI->create_view(VK_IMAGE_ASPECT_COLOR_BIT);

        auto buf = vkd::begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());

        fontI->set_layout(buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        fontI->copy(*staging, buf);

        fontI->set_layout(buf, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        vkd::flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);

        _font_images.push_back(fontI);
        io.Fonts->TexID = (void*)(_font_images.size() - 1);

        // Cleanup (don't clear the input data if you want to append new fonts later)
        io.Fonts->ClearInputData();
        io.Fonts->ClearTexData();
        */
    }
}