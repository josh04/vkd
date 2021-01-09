#include "kernel.hpp"
#include "vulkan.hpp"
#include "descriptor_sets.hpp"
#include "compute_pipeline.hpp"
#include "shader.hpp"
#include "image.hpp"
#include "buffer.hpp"

namespace vulkan {
    void Kernel::init(std::string path, std::string func_name, std::array<int32_t, 3> local_sizes) {
        memset(&_constants, 0, sizeof(ExecutionConstants));
        _pipeline_cache = std::make_shared<PipelineCache>(_device);
        _pipeline_cache->create();

		_shader = std::make_unique<ComputeShader>(_device);
        _shader->create(path, func_name);

        _desc_pool = std::make_shared<DescriptorPool>(_device);
        auto counts = _shader->descriptor_counts();
        _desc_pool->add_uniform_buffer(counts.uniform_buffers);
        _desc_pool->add_storage_buffer(counts.storage_buffers);
        _desc_pool->add_combined_image_sampler(counts.image_samplers);
        _desc_pool->add_storage_image(counts.storage_images);
        _desc_pool->create(1);
        
        _desc_set_layout = _shader->desc_set_layout();

        for (auto&& constant : _shader->push_constants()) {
            _push_constant_size = constant.size;
        }

        _local_group_sizes = local_sizes;

        for (auto&& pair : _shader->push_constant_types()) {
            std::shared_ptr<ParameterInterface> param = nullptr;
            switch (pair.type) {
            case ParameterType::p_float:
                param = make_param<ParameterType::p_float>(pair.name, pair.offset);
                break;
            case ParameterType::p_int:
                param = make_param<ParameterType::p_int>(pair.name, pair.offset);
                break;
            case ParameterType::p_uint:
                param = make_param<ParameterType::p_uint>(pair.name, pair.offset);
                break;
            case ParameterType::p_vec2:
                param = make_param<ParameterType::p_vec2>(pair.name, pair.offset);
                break;
            case ParameterType::p_vec4:
                param = make_param<ParameterType::p_vec4>(pair.name, pair.offset);
                break;
            case ParameterType::p_ivec2:
                param = make_param<ParameterType::p_ivec2>(pair.name, pair.offset);
                break;
            case ParameterType::p_ivec4:
                param = make_param<ParameterType::p_ivec4>(pair.name, pair.offset);
                break;
            case ParameterType::p_uvec2:
                param = make_param<ParameterType::p_uvec2>(pair.name, pair.offset);
                break;
            case ParameterType::p_uvec4:
                param = make_param<ParameterType::p_uvec4>(pair.name, pair.offset);
                break;
            }
            _params.emplace(pair.name, param);
            if (pair.name.substr(0, 1) != "_" && pair.name.substr(0, 4) != "vkd_") {
                _public_params.emplace(pair.name, param);
            }
        }
        
        _create_pipeline(_full_pipeline, _local_group_sizes);
    }

    void Kernel::_create_pipeline(PartialPipeline& pipeline, std::array<int32_t, 3> local_sizes) {
        if (local_sizes[0] < 1 || local_sizes[1] < 1 || local_sizes[2] < 1) {
            throw std::runtime_error("Bad pipeline local size");
        }

        pipeline.pipeline = std::make_shared<ComputePipeline>(_device, _pipeline_cache);

        for (auto&& constant : _shader->push_constants()) {
            pipeline.pipeline->layout()->add_constant(constant);
        }
        pipeline.local_sizes = local_sizes;
        pipeline.pipeline->create(_desc_set_layout->get(), _shader.get(), local_sizes);
    }

    void Kernel::set_arg(int32_t index, std::shared_ptr<Buffer> buffer) {
        _args[index] = Arg{buffer, nullptr};
        _args_changed = true;
    }
    
    void Kernel::set_arg(int32_t index, std::shared_ptr<Image> image) {
        _args[index] = Arg{nullptr, image};
        _args_changed = true;
    }

    void Kernel::set_offset(int32_t x, int32_t y, int32_t z, int32_t w) {
        _constants.offset.x = x;
        _constants.offset.y = y;
        _constants.offset.z = z;
        _constants.offset.w = w;
    }

    void Kernel::update() {
        if (_args_changed) {
            _desc_set = std::make_shared<DescriptorSet>(_device, _desc_set_layout, _desc_pool);
            for (auto&& arg : _args) {
                if (arg.second.buffer) {
                    _desc_set->add_buffer(*arg.second.buffer);
                } else if (arg.second.image) {
                    _desc_set->add_image(*arg.second.image, arg.second.image->sampler());
                }
            }
            _desc_set->create();

            _args_changed = false;
        }
    }

    namespace {
        constexpr bool debug = false;
        glm::ivec4 last_offset = {0,0,0,0};
    }

    void Kernel::_push_execution_offset(VkCommandBuffer buf, const glm::ivec4& offset) {
        ExecutionConstants constants = _constants;
        constants.offset = _constants.offset + offset;
        if constexpr (debug) { last_offset = constants.offset; }

        set_push_arg_by_name(buf, "vkd_offset", constants.offset);
    }

    void debug_print(std::string name, int32_t g_x, int32_t g_y, int32_t g_z) {
        if constexpr (debug) { std::cout << "dispatching " << name << " x: " << g_x << " y: " << g_y << " z: " << g_z
            << " offsets x: " << last_offset.x << " offsets y: " << last_offset.y << " z: " << last_offset.z << std::endl; }
    }

	void Kernel::dispatch(VkCommandBuffer buf, int32_t global_x, int32_t global_y, int32_t global_z) {
        _full_pipeline.pipeline->bind(buf, _desc_set->get());

        auto&& local_group_sizes_ = _full_pipeline.local_sizes;

		int32_t trim_x = (global_x / local_group_sizes_[0]);
		int32_t trim_y = (global_y / local_group_sizes_[1]);
		int32_t trim_z = (global_z / local_group_sizes_[2]);

		int32_t overflow_x = (global_x % local_group_sizes_[0]);
		int32_t overflow_y = (global_y % local_group_sizes_[1]);
		int32_t overflow_z = (global_z % local_group_sizes_[2]);

		int32_t full_x = global_x - overflow_x;
		int32_t full_y = global_y - overflow_y;
		int32_t full_z = global_z - overflow_z;

        for (auto&& param : _params) {
            //if (param.second->changed()) {
                std::cout << _shader->path() << ": " << param.second->name() << " size: " << param.second->size() << " offset: " << param.second->offset() << std::endl;
                set_push_arg(buf, *param.second);
            //}
        }

        debug_print(_shader->path(), trim_x*local_group_sizes_[0], trim_y*local_group_sizes_[1], trim_z*local_group_sizes_[2]);
        _push_execution_offset(buf, {0,0,0,0});

		vkCmdDispatch(buf, trim_x, trim_y, trim_z);

		if (overflow_x > 0) {
            _push_execution_offset(buf, {full_x, 0, 0, 0});
			_dispatch_overflow(buf, {overflow_x, local_group_sizes_[1], local_group_sizes_[2]}, {1, trim_y, trim_z});

			if (overflow_y > 0) {
                _push_execution_offset(buf, {full_x, full_y, 0, 0});
				_dispatch_overflow(buf, {overflow_x, overflow_y, local_group_sizes_[2]}, {1, 1, trim_z});

				if (overflow_z > 0) {
                    _push_execution_offset(buf, {full_x, full_x, full_z, 0});
					_dispatch_overflow(buf, {overflow_x, overflow_y, overflow_z}, {1, 1, 1});
				}
			}

			if (overflow_z > 0) {
                _push_execution_offset(buf, {full_x, 0, full_z, 0});
				_dispatch_overflow(buf, {overflow_x, local_group_sizes_[1], overflow_z}, {1, trim_y, 1});
			}
		}

		if (overflow_y > 0) {
            _push_execution_offset(buf, {0, full_y, 0, 0});
			_dispatch_overflow(buf, {local_group_sizes_[0], overflow_y, local_group_sizes_[2]}, {trim_x, 1, trim_z});

			if (overflow_z > 0) {
                _push_execution_offset(buf, {0, full_y, full_z, 0});
				_dispatch_overflow(buf, {local_group_sizes_[0], overflow_y, overflow_z}, {trim_x, 1, 1});
			}
		}

		if (overflow_z > 0) {
            _push_execution_offset(buf, {0, 0, full_z, 0});
			_dispatch_overflow(buf, {local_group_sizes_[0], local_group_sizes_[1], overflow_z}, {trim_x, trim_y, 1});
		}

        std::vector<VkBufferMemoryBarrier> membar;
        std::vector<VkImageMemoryBarrier> imagebar;
        for (auto&& mem : _args) {
            if (mem.second.buffer) {
                membar.push_back(mem.second.buffer->barrier_info(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
            } else if (mem.second.image) {
                imagebar.push_back(mem.second.image->barrier_info(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
            }
        }
		
        vkCmdPipelineBarrier(
            buf,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0,
            0, nullptr,
            membar.size(), membar.data(),
            imagebar.size(), imagebar.data());
	}

    void Kernel::_dispatch_overflow(VkCommandBuffer buf, std::array<int32_t, 3> size, std::array<int32_t, 3> count) {
        bool is_x = (size[0] != _local_group_sizes[0]);
        bool is_y = (size[1] != _local_group_sizes[1]);
        bool is_z = (size[2] != _local_group_sizes[2]);

        std::shared_ptr<ComputePipeline> pipeline = nullptr;
        std::array<int32_t, 3> local_sizes = {0,0,0};

        if (is_x && is_y && is_z) {
            if (_overflow_xyz_pipeline.local_sizes != size) {
                _create_pipeline(_overflow_xyz_pipeline, size);
            }
            pipeline = _overflow_xyz_pipeline.pipeline;
            local_sizes = _overflow_xyz_pipeline.local_sizes;
        } else if (is_x && is_y) {
            if (_overflow_xy_pipeline.local_sizes != size) {
                _create_pipeline(_overflow_xy_pipeline, size);
            }
            pipeline = _overflow_xy_pipeline.pipeline;
            local_sizes = _overflow_xy_pipeline.local_sizes;
        } else if (is_x && is_z) {
            if (_overflow_xz_pipeline.local_sizes != size) {
                _create_pipeline(_overflow_xz_pipeline, size);
            }
            pipeline = _overflow_xz_pipeline.pipeline;
            local_sizes = _overflow_xz_pipeline.local_sizes;
        } else if (is_y && is_z) {
            if (_overflow_yz_pipeline.local_sizes != size) {
                _create_pipeline(_overflow_yz_pipeline, size);
            }
            pipeline = _overflow_yz_pipeline.pipeline;
            local_sizes = _overflow_yz_pipeline.local_sizes;
        } else if (is_x) {
            if (_overflow_x_pipeline.local_sizes != size) {
                _create_pipeline(_overflow_x_pipeline, size);
            }
            pipeline = _overflow_x_pipeline.pipeline;
            local_sizes = _overflow_x_pipeline.local_sizes;
        } else if (is_y) {
            if (_overflow_y_pipeline.local_sizes != size) {
                _create_pipeline(_overflow_y_pipeline, size);
            }
            pipeline = _overflow_y_pipeline.pipeline;
            local_sizes = _overflow_y_pipeline.local_sizes;
        } else if (is_z) {
            if (_overflow_z_pipeline.local_sizes != size) {
                _create_pipeline(_overflow_z_pipeline, size);
            }
            pipeline = _overflow_z_pipeline.pipeline;
            local_sizes = _overflow_z_pipeline.local_sizes;
        } else {
            return;
        }

        pipeline->bind(buf, _desc_set->get());
        debug_print(_shader->path(), count[0]*local_sizes[0], count[1]*local_sizes[1], count[2]*local_sizes[2]);
        vkCmdDispatch(buf, count[0], count[1], count[2]);
    }
}