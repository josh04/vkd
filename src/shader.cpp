#include "shader.hpp"
#include "spirv.hpp"
#include "device.hpp"
#include "descriptor_sets.hpp"

namespace vulkan {
    Shader::~Shader() {
        for (auto&& stage : _stages) {
		    vkDestroyShaderModule(_device->logical_device(), stage.module, nullptr);
        }
    }

    void Shader::add(VkShaderStageFlagBits stage, const std::string& path, const std::string& main_name) {
		// Shaders
		VkPipelineShaderStageCreateInfo stage_create_info = {};

		// Vertex shader
		stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		// Set pipeline stage for this shader
		stage_create_info.stage = stage;
		// Load binary SPIR-V shader
		stage_create_info.module = load_spirv_shader(_device->logical_device(), path, nullptr);
		// Main entry point for the shader
        _main_names.emplace_back(std::make_unique<std::string>(main_name));
		stage_create_info.pName = _main_names.back()->c_str();
		assert(stage_create_info.module != VK_NULL_HANDLE);

        _stages.push_back(stage_create_info);
    }

    ComputeShader::~ComputeShader() {
		vkDestroyShaderModule(_device->logical_device(), _stage.module, nullptr);
    }

    void ComputeShader::create(const std::string& path, const std::string& main_name) {
		_path = path;
		_main_name = main_name;
		_stage = {};

		_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		_stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		SpvReflectShaderModule reflection_module;
		_stage.module = load_spirv_shader(_device->logical_device(), path, &reflection_module);

		_stage.pName = _main_name.c_str();
		if (_stage.module == VK_NULL_HANDLE) {
			throw std::runtime_error(std::string("Shader failed to compile: ") + path);
		}
		
		_reflect(reflection_module);
    }

	void ComputeShader::_reflect(SpvReflectShaderModule reflection_module) {
		
		uint32_t count = 0;
		SpvReflectResult result = spvReflectEnumerateDescriptorSets(&reflection_module, &count, NULL);
		if (result != SPV_REFLECT_RESULT_SUCCESS) {
			throw std::runtime_error("Failed to enumerate descriptor sets in shader.");
		}

		std::vector<SpvReflectDescriptorSet*> desc_sets(count);
		result = spvReflectEnumerateDescriptorSets(&reflection_module, &count, desc_sets.data());
		if (result != SPV_REFLECT_RESULT_SUCCESS) {
			throw std::runtime_error("Failed to enumerate descriptor sets in shader.");
		}

		if (desc_sets.size() > 1) {
			throw std::runtime_error("Too many descriptor sets in shader.");
		}

		for (auto set_ptr : desc_sets) {
			const SpvReflectDescriptorSet& set = *set_ptr;

			SetLayoutData layout;
			
			layout.bindings.resize(set.binding_count);

			for (uint32_t i_binding = 0; i_binding < set.binding_count; ++i_binding) {

				const SpvReflectDescriptorBinding& refl_binding = *(set.bindings[i_binding]);
				VkDescriptorSetLayoutBinding& layout_binding = layout.bindings[i_binding];

				layout_binding.binding = refl_binding.binding;
				layout_binding.descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);
				layout_binding.descriptorCount = 1;

				for (uint32_t i = 0; i < refl_binding.array.dims_count; ++i) {
					layout_binding.descriptorCount *= refl_binding.array.dims[i];
				}

				layout_binding.stageFlags = static_cast<VkShaderStageFlagBits>(reflection_module.shader_stage);
			}
			
			layout.set_number = set.set;
			layout.create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layout.create_info.bindingCount = set.binding_count;
			layout.create_info.pBindings = layout.bindings.data();

			_set_layouts.push_back(layout);
		}
		
		const SpvReflectEntryPoint * entry_point = spvReflectGetEntryPoint(&reflection_module, _main_name.c_str());
		if (!entry_point) {
			throw std::runtime_error("Failed to get spirv entry point.");
		}
		
		_local_group_sizes[0] = entry_point->local_size.x;
		_local_group_sizes[1] = entry_point->local_size.y;
		_local_group_sizes[2] = entry_point->local_size.z;

		uint32_t const_count = 0;
		SpvReflectResult result_cons = spvReflectEnumerateEntryPointPushConstantBlocks(&reflection_module, _main_name.c_str(), &const_count, nullptr);
		if (result_cons != SPV_REFLECT_RESULT_SUCCESS) {
			throw std::runtime_error("Failed to enumerate push constants in shader.");
		}
		std::vector<SpvReflectBlockVariable*> blocks;
		blocks.resize(const_count);

		if (const_count > 0) {
			result_cons = spvReflectEnumerateEntryPointPushConstantBlocks(&reflection_module, _main_name.c_str(), &const_count, blocks.data());
			if (result_cons != SPV_REFLECT_RESULT_SUCCESS) {
				throw std::runtime_error("Failed to enumerate push constants in shader.");
			}

			for (uint32_t i = 0; i < const_count; ++i) {
				auto&& block = *(blocks[i]);
				VkPushConstantRange constant = {};
				constant.stageFlags = static_cast<VkShaderStageFlagBits>(reflection_module.shader_stage);
				constant.size = block.size;
				constant.offset = block.offset;
				if (constant.size > 0) {
					_push_constants.push_back(constant);
				}

				if (block.type_description != nullptr) {
					assert(block.type_description->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT);
					size_t soffset = 0;
					for (int i = 0; i < block.type_description->member_count; ++i) {
						auto&& member = block.type_description->members[i];
						ParameterPair pair;
						pair.name = member.struct_member_name;
						pair.offset = soffset;
						if (member.type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {
							if (member.type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
								if (member.traits.numeric.vector.component_count == 2) {
									pair.type = ParameterType::p_vec2;
										soffset += sizeof(glm::vec2);
								} else {
									pair.type = ParameterType::p_vec4;
										soffset += sizeof(glm::vec4);
								}
							} else if (member.type_flags & SPV_REFLECT_TYPE_FLAG_INT) {
								if (member.traits.numeric.scalar.signedness == 0) {
									if (member.traits.numeric.vector.component_count == 2) {
										pair.type = ParameterType::p_uvec2;
										soffset += sizeof(glm::ivec2);
									} else {
										pair.type = ParameterType::p_uvec4;
										soffset += sizeof(glm::ivec4);
									}
								} else {
									if (member.traits.numeric.vector.component_count == 2) {
										pair.type = ParameterType::p_ivec2;
										soffset += sizeof(glm::uvec2);
									} else {
										pair.type = ParameterType::p_ivec4;
										soffset += sizeof(glm::uvec4);
									}
								}
							}
						} else if (member.type_flags == SPV_REFLECT_TYPE_FLAG_FLOAT) {
							pair.type = ParameterType::p_float;
							soffset += sizeof(float);
						} else if (member.type_flags == SPV_REFLECT_TYPE_FLAG_INT) {
							if (member.traits.numeric.scalar.signedness == 0) {
								pair.type = ParameterType::p_uint;
								soffset += sizeof(uint32_t);
							} else {
								pair.type = ParameterType::p_int;
								soffset += sizeof(int32_t);
							}
						} else {
							throw std::runtime_error("Unexpected push constant layout.");
						}
						_types.push_back(pair);
					}
				}

				//std::cout << _path << " constant size: " << constant.size << " offset: " << constant.offset << " stage: " << constant.stageFlags << std::endl; 	
			}
		}

		spvReflectDestroyShaderModule(&reflection_module);
	}
    
	std::shared_ptr<DescriptorLayout> ComputeShader::desc_set_layout() const {
        auto desc_set_layout = std::make_shared<DescriptorLayout>(_device);

		if (_set_layouts.size() > 1) {
			throw std::runtime_error("Too many descriptor sets in shader.");
		}

		for (auto&& layout : _set_layouts) {
			for (auto&& binding : layout.bindings) {
				desc_set_layout->add(binding.descriptorType, binding.descriptorCount, binding.stageFlags);
			}
		}

        desc_set_layout->create();

		return desc_set_layout;
	}

	ComputeShader::DescriptorCounts ComputeShader::descriptor_counts() const {
		DescriptorCounts counts;
		for (auto&& layout : _set_layouts) {
			for (auto&& binding : layout.bindings) {
				switch (binding.descriptorType) {
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
					counts.image_samplers += binding.descriptorCount;
					break;
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
					counts.storage_images += binding.descriptorCount;
					break;
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
					counts.uniform_buffers += binding.descriptorCount;
					break;
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
					counts.storage_buffers += binding.descriptorCount;
					break;
				}
			}
		}
		return counts;
	}
}