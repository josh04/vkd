#include "sand.hpp"
#include <random>
#include "command_buffer.hpp"
#include "device.hpp"
#include "pipeline.hpp"
#include "shader.hpp"
#include "vulkan.hpp"
#include "kernel.hpp"
#include "image.hpp"
#include "buffer.hpp"

#include <random>
namespace {
    std::default_random_engine engine(0);// : (unsigned)time(nullptr));
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::uniform_int_distribution<int> dist2(-10,10);
}

namespace vkd {
    REGISTER_NODE("sand", "sand", Sand, 1280, 720);

    Sand::Sand(int32_t width, int32_t height) : _width(width), _height(height) {}

    void Sand::init() {

        
        
        /// START RESOURCE ACQ

		VkDeviceSize sand_add_size = _width * sizeof(uint32_t);
        _sand_add_locations = std::make_shared<Buffer>(_device);
        _sand_add_locations->init(sand_add_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        
        _add_loc_stage = std::make_shared<StagingBuffer>(_device);
        _add_loc_stage->create(sand_add_size);
        _add_loc_map = (uint32_t*)_add_loc_stage->map();

        update(ExecutionType::UI);

        _sand_locations = std::make_shared<vkd::Image>(_device);
        _sand_locations->create_image(VK_FORMAT_R32G32B32A32_UINT, {_width, _height}, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        _sand_locations->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        _sand_locations->create_view(VK_IMAGE_ASPECT_COLOR_BIT);

        _sand_locations_sc = std::make_shared<vkd::Image>(_device);
        _sand_locations_sc->create_image(VK_FORMAT_R32G32B32A32_UINT, {_width, _height}, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        _sand_locations_sc->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        _sand_locations_sc->create_view(VK_IMAGE_ASPECT_COLOR_BIT);

        _sand_locations_scratch = std::make_shared<vkd::Image>(_device);
        _sand_locations_scratch->create_image(VK_FORMAT_R32G32B32A32_UINT, {_width, _height}, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        _sand_locations_scratch->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        _sand_locations_scratch->create_view(VK_IMAGE_ASPECT_COLOR_BIT);

        _sand_image = std::make_shared<vkd::Image>(_device);
        _sand_image->create_image(VK_FORMAT_R32G32B32A32_SFLOAT, {_width, _height}, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT );
        _sand_image->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        _sand_image->create_view(VK_IMAGE_ASPECT_COLOR_BIT);

        auto buf = vkd::begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());

        _sand_locations->set_layout(buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        _sand_locations_scratch->set_layout(buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        _sand_locations_sc->set_layout(buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        _sand_image->set_layout(buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        vkd::flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);

        /// END RESOURCE ACQ
        _sand_clear = std::make_shared<Kernel>(_device, param_hash_name());
        _sand_clear->init("shaders/compute/sand_clear.comp.spv", "main", Kernel::default_local_sizes);
        _sand_clear->set_arg(0, _sand_locations);

        _sand_add_bumpers = std::make_shared<Kernel>(_device, param_hash_name());
        _sand_add_bumpers->init("shaders/compute/sand_add_bumpers.comp.spv", "main", {256, 1, 1});
        _sand_add_bumpers->set_arg(0, _sand_locations);

        _sand_bottom_bump = std::make_shared<Kernel>(_device, param_hash_name());
        _sand_bottom_bump->init("shaders/compute/sand_bottom_bump.comp.spv", "main", {256, 1, 1});
        _sand_bottom_bump->set_arg(0, _sand_locations);
        _sand_bottom_bump->set_arg(1, _sand_locations_scratch);
        _sand_bottom_bump->set_arg(2, _sand_locations_sc);

        _sand_process = std::make_shared<Kernel>(_device, param_hash_name());
        _sand_process->init("shaders/compute/sand_process.comp.spv", "main", Kernel::default_local_sizes);
        _sand_process->set_arg(0, _sand_locations);
        _sand_process->set_arg(1, _sand_locations_sc);
        _sand_process->set_arg(2, _sand_locations_scratch);
        
        _sand_copy_stills = std::make_shared<Kernel>(_device, param_hash_name());
        _sand_copy_stills->init("shaders/compute/sand_copy_stills.comp.spv", "main", Kernel::default_local_sizes);
        _sand_copy_stills->set_arg(0, _sand_locations);
        _sand_copy_stills->set_arg(1, _sand_locations_scratch);

        _sand_add = std::make_shared<Kernel>(_device, param_hash_name());
        _sand_add->init("shaders/compute/sand_add.comp.spv", "main", Kernel::default_local_sizes);
        _sand_add->set_arg(0, _sand_add_locations);
        _sand_add->set_arg(1, _sand_locations_scratch);

        _sand_to_image = std::make_shared<Kernel>(_device, param_hash_name());
        _sand_to_image->init("shaders/compute/sand_to_image.comp.spv", "main", Kernel::default_local_sizes);
        register_params(*_sand_to_image);
        _sand_to_image->set_arg(0, _sand_locations_scratch);
        _sand_to_image->set_arg(1, _sand_image);

        auto buf2 = vkd::begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());

        _sand_clear->dispatch(buf2, _width, _height);
        _sand_add_bumpers->dispatch(buf2, _width, _height);

        vkd::flush_command_buffer(_device->logical_device(), _device->compute_queue(), _device->command_pool(), buf2);

        _sand_clear->set_arg(0, _sand_locations_scratch);
    }

    bool Sand::update(ExecutionType type) {
        memset(_add_loc_map, 0, sizeof(uint32_t) * _width);
        for (int i = 1; i < _width; ++i) {
            if (i % (_width/5) == 0) {
                int off = std::max(std::min(i + dist2(engine), (int)_width), 0);
                _add_loc_map[off] = 1;
                //console << "rand: " << off << std::endl;
            }
        }
        
        bool update = false;
        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }

        if (update) {
            command_buffer().begin();
            
            _sand_clear->dispatch(command_buffer(), _width, _height);

            glm::ivec2 offs = {0, _height - 1};
            _sand_bottom_bump->set_offset(offs);

            _sand_bottom_bump->dispatch(command_buffer(), _width, 1);

            _ubo.wobble = 0;
            _sand_process->set_push_arg_by_name(command_buffer().get(), "_wobble", _ubo.wobble);
            _sand_process->dispatch(command_buffer(), _width, _height-1);
            _sand_locations->copy(*_sand_locations_sc, command_buffer().get());
            
            _ubo.wobble = -1;
            _sand_process->set_push_arg_by_name(command_buffer().get(), "_wobble", _ubo.wobble);
            _sand_process->dispatch(command_buffer(), _width, _height-1);
            _sand_locations->copy(*_sand_locations_sc, command_buffer().get());

            _ubo.wobble = 1;
            _sand_process->set_push_arg_by_name(command_buffer().get(), "_wobble", _ubo.wobble);
            _sand_process->dispatch(command_buffer(), _width, _height-1);
            _sand_locations->copy(*_sand_locations_sc, command_buffer().get());

            _sand_copy_stills->dispatch(command_buffer(), _width, _height-1);

            _sand_add_locations->copy(*_add_loc_stage, _sand_add_locations->requested_size(), command_buffer().get());
            _sand_add_locations->barrier(command_buffer().get(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            
            _sand_add->dispatch(command_buffer(), _width, 1);
            _sand_to_image->dispatch(command_buffer(), _width, _height);

            _sand_locations->copy(*_sand_locations_scratch, command_buffer().get());
            _sand_locations_scratch->barrier(command_buffer().get(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            _sand_locations->barrier(command_buffer().get(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

            command_buffer().end();

        }

        return update;
    }

    void Sand::execute(ExecutionType type, Stream& stream) {
        stream.submit(command_buffer());
    }
}