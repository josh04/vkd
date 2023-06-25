#include "shader.hpp"
#include "spirv.hpp"
#include "device.hpp"
#include "descriptor_sets.hpp"

#include "glslang/Public/ShaderLang.h"
#include "glslang/SPIRV/GlslangToSpv.h"

namespace vkd {
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

				layout.binding_names[refl_binding.name] = i_binding;

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
						auto&& member_desc = block.type_description->members[i];
						auto&& member_block = block.members[i];
						ParameterPair pair;
						pair.name = member_desc.struct_member_name;
						pair.offset = member_block.offset;
						if (member_desc.type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {
							if (member_desc.type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
								if (member_desc.traits.numeric.vector.component_count == 2) {
									pair.type = ParameterType::p_vec2;
								} else {
									pair.type = ParameterType::p_vec4;
								}
							} else if (member_desc.type_flags & SPV_REFLECT_TYPE_FLAG_INT) {
								if (member_desc.traits.numeric.scalar.signedness == 0) {
									if (member_desc.traits.numeric.vector.component_count == 2) {
										pair.type = ParameterType::p_uvec2;
									} else {
										pair.type = ParameterType::p_uvec4;
									}
								} else {
									if (member_desc.traits.numeric.vector.component_count == 2) {
										pair.type = ParameterType::p_ivec2;
									} else {
										pair.type = ParameterType::p_ivec4;
									}
								}
							}
						} else if (member_desc.type_flags == SPV_REFLECT_TYPE_FLAG_FLOAT) {
							pair.type = ParameterType::p_float;
						} else if (member_desc.type_flags == SPV_REFLECT_TYPE_FLAG_INT) {
							if (member_desc.traits.numeric.scalar.signedness == 0) {
								pair.type = ParameterType::p_uint;
							} else {
								pair.type = ParameterType::p_int;
							}
						} else if (member_desc.type_flags == SPV_REFLECT_TYPE_FLAG_BOOL) {
							pair.type = ParameterType::p_bool;
						} else {
							throw std::runtime_error("Unexpected push constant layout.");
						}
						_types.push_back(pair);
					}
				}

				//console << _path << " constant size: " << constant.size << " offset: " << constant.offset << " stage: " << constant.stageFlags << std::endl; 	
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

	ManualComputeShader::~ManualComputeShader() {}

	namespace {
		const TBuiltInResource DefaultTBuiltInResource = {
			/* .MaxLights = */ 32,
			/* .MaxClipPlanes = */ 6,
			/* .MaxTextureUnits = */ 32,
			/* .MaxTextureCoords = */ 32,
			/* .MaxVertexAttribs = */ 64,
			/* .MaxVertexUniformComponents = */ 4096,
			/* .MaxVaryingFloats = */ 64,
			/* .MaxVertexTextureImageUnits = */ 32,
			/* .MaxCombinedTextureImageUnits = */ 80,
			/* .MaxTextureImageUnits = */ 32,
			/* .MaxFragmentUniformComponents = */ 4096,
			/* .MaxDrawBuffers = */ 32,
			/* .MaxVertexUniformVectors = */ 128,
			/* .MaxVaryingVectors = */ 8,
			/* .MaxFragmentUniformVectors = */ 16,
			/* .MaxVertexOutputVectors = */ 16,
			/* .MaxFragmentInputVectors = */ 15,
			/* .MinProgramTexelOffset = */ -8,
			/* .MaxProgramTexelOffset = */ 7,
			/* .MaxClipDistances = */ 8,
			/* .MaxComputeWorkGroupCountX = */ 65535,
			/* .MaxComputeWorkGroupCountY = */ 65535,
			/* .MaxComputeWorkGroupCountZ = */ 65535,
			/* .MaxComputeWorkGroupSizeX = */ 1024,
			/* .MaxComputeWorkGroupSizeY = */ 1024,
			/* .MaxComputeWorkGroupSizeZ = */ 64,
			/* .MaxComputeUniformComponents = */ 1024,
			/* .MaxComputeTextureImageUnits = */ 16,
			/* .MaxComputeImageUniforms = */ 8,
			/* .MaxComputeAtomicCounters = */ 8,
			/* .MaxComputeAtomicCounterBuffers = */ 1,
			/* .MaxVaryingComponents = */ 60,
			/* .MaxVertexOutputComponents = */ 64,
			/* .MaxGeometryInputComponents = */ 64,
			/* .MaxGeometryOutputComponents = */ 128,
			/* .MaxFragmentInputComponents = */ 128,
			/* .MaxImageUnits = */ 8,
			/* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
			/* .MaxCombinedShaderOutputResources = */ 8,
			/* .MaxImageSamples = */ 0,
			/* .MaxVertexImageUniforms = */ 0,
			/* .MaxTessControlImageUniforms = */ 0,
			/* .MaxTessEvaluationImageUniforms = */ 0,
			/* .MaxGeometryImageUniforms = */ 0,
			/* .MaxFragmentImageUniforms = */ 8,
			/* .MaxCombinedImageUniforms = */ 8,
			/* .MaxGeometryTextureImageUnits = */ 16,
			/* .MaxGeometryOutputVertices = */ 256,
			/* .MaxGeometryTotalOutputComponents = */ 1024,
			/* .MaxGeometryUniformComponents = */ 1024,
			/* .MaxGeometryVaryingComponents = */ 64,
			/* .MaxTessControlInputComponents = */ 128,
			/* .MaxTessControlOutputComponents = */ 128,
			/* .MaxTessControlTextureImageUnits = */ 16,
			/* .MaxTessControlUniformComponents = */ 1024,
			/* .MaxTessControlTotalOutputComponents = */ 4096,
			/* .MaxTessEvaluationInputComponents = */ 128,
			/* .MaxTessEvaluationOutputComponents = */ 128,
			/* .MaxTessEvaluationTextureImageUnits = */ 16,
			/* .MaxTessEvaluationUniformComponents = */ 1024,
			/* .MaxTessPatchComponents = */ 120,
			/* .MaxPatchVertices = */ 32,
			/* .MaxTessGenLevel = */ 64,
			/* .MaxViewports = */ 16,
			/* .MaxVertexAtomicCounters = */ 0,
			/* .MaxTessControlAtomicCounters = */ 0,
			/* .MaxTessEvaluationAtomicCounters = */ 0,
			/* .MaxGeometryAtomicCounters = */ 0,
			/* .MaxFragmentAtomicCounters = */ 8,
			/* .MaxCombinedAtomicCounters = */ 8,
			/* .MaxAtomicCounterBindings = */ 1,
			/* .MaxVertexAtomicCounterBuffers = */ 0,
			/* .MaxTessControlAtomicCounterBuffers = */ 0,
			/* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
			/* .MaxGeometryAtomicCounterBuffers = */ 0,
			/* .MaxFragmentAtomicCounterBuffers = */ 1,
			/* .MaxCombinedAtomicCounterBuffers = */ 1,
			/* .MaxAtomicCounterBufferSize = */ 16384,
			/* .MaxTransformFeedbackBuffers = */ 4,
			/* .MaxTransformFeedbackInterleavedComponents = */ 64,
			/* .MaxCullDistances = */ 8,
			/* .MaxCombinedClipAndCullDistances = */ 8,
			/* .MaxSamples = */ 4,
			/* .maxMeshOutputVerticesNV = */ 256,
			/* .maxMeshOutputPrimitivesNV = */ 512,
			/* .maxMeshWorkGroupSizeX_NV = */ 32,
			/* .maxMeshWorkGroupSizeY_NV = */ 1,
			/* .maxMeshWorkGroupSizeZ_NV = */ 1,
			/* .maxTaskWorkGroupSizeX_NV = */ 32,
			/* .maxTaskWorkGroupSizeY_NV = */ 1,
			/* .maxTaskWorkGroupSizeZ_NV = */ 1,
			/* .maxMeshViewCountNV = */ 4,
			/* .maxDualSourceDrawBuffersEXT = */ 1,

			/* .limits = */ {
				/* .nonInductiveForLoops = */ 1,
				/* .whileLoops = */ 1,
				/* .doWhileLoops = */ 1,
				/* .generalUniformIndexing = */ 1,
				/* .generalAttributeMatrixVectorIndexing = */ 1,
				/* .generalVaryingIndexing = */ 1,
				/* .generalSamplerIndexing = */ 1,
				/* .generalVariableIndexing = */ 1,
				/* .generalConstantMatrixVectorIndexing = */ 1,
		}};

	}

    void ManualComputeShader::create(const std::string& kernel, const std::string& main_name) {

        ShInitialize();

		_path = "MANUAL_SHADER";
		_main_name = main_name;
		_stage = {};

		std::unique_ptr<glslang::TShader> tshader = std::make_unique<glslang::TShader>(EShLangCompute);

		int kernel_length = (int)kernel.size();
		const char * kernel_char = kernel.c_str();
		const char * kernel_name = "MODULE_MANUAL";
        tshader->setStringsWithLengthsAndNames(&kernel_char, &kernel_length, &kernel_name, 1);
		tshader->setEntryPoint(main_name.c_str());

#ifdef __APPLE__
		tshader->setEnvInput(glslang::EShSourceGlsl, EShLangCompute, glslang::EShClientVulkan, 150);
		tshader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
		tshader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
#elif _WIN32
		tshader->setEnvInput(glslang::EShSourceGlsl, EShLangCompute, glslang::EShClientVulkan, 150);
		tshader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
		tshader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
#else
		tshader->setEnvInput(glslang::EShSourceGlsl, EShLangCompute, glslang::EShClientVulkan, 100);
		tshader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
		tshader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
#endif

		tshader->setAutoMapBindings(true);
		tshader->setAutoMapLocations(true);

		const int defaultVersion = 110;
		EShMessages messages = EShMsgDefault;
        glslang::TShader::ForbidIncluder includer;

		/*
    GLSLANG_EXPORT bool preprocess(
        const TBuiltInResource* builtInResources, int defaultVersion,
        EProfile defaultProfile, bool forceDefaultVersionAndProfile,
        bool forwardCompatible, EShMessages message, std::string* outputString,
        Includer& includer);*/
/*
		std::string output;
		if (!tshader->preprocess(&DefaultTBuiltInResource, defaultVersion, ENoProfile, false, false, messages, &output, includer)) {
			console << tshader->getInfoDebugLog() << std::endl;
            throw std::runtime_error("Shader failed to preprocess.");
		}
		console << output << std::endl;
*/
        if (!tshader->parse(&DefaultTBuiltInResource, defaultVersion, false, messages, includer)) {
			console << tshader->getInfoDebugLog() << std::endl;
            throw std::runtime_error("Shader failed to parse."); // FIXME make not a runtime error, provide feedback
		}


		std::unique_ptr<glslang::TProgram> tprogram = std::make_unique<glslang::TProgram>();

		tprogram->addShader(tshader.get());

		// Link
		if (!tprogram->link(messages) || !tprogram->mapIO()) {
            throw std::runtime_error("Shader program failed to link and/or map.");
		}

		std::vector<uint32_t> spirv;

		spv::SpvBuildLogger logger;
		glslang::SpvOptions spvOptions;

		constexpr bool build_debug = true;
		if (build_debug) {
			spvOptions.generateDebugInfo = build_debug;
		} else {
			spvOptions.stripDebugInfo = true;
		}

		spvOptions.disableOptimizer = false;
		spvOptions.optimizeSize = true;
		spvOptions.disassemble = false;
		spvOptions.validate = false;
		glslang::GlslangToSpv(*tprogram->getIntermediate(EShLangCompute), spirv, &logger, &spvOptions);

		// Create a new shader module that will be used for pipeline creation
		VkShaderModuleCreateInfo module_create_info{};
		module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		module_create_info.codeSize = spirv.size() * sizeof(uint32_t);
		module_create_info.pCode = (uint32_t*)spirv.data();

		_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		_stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		SpvReflectShaderModule reflection_module;

		_stage.module = load_spirv_shader(_device->logical_device(), "MANUAL", spirv, &reflection_module);

		_stage.pName = _main_name.c_str();
		if (_stage.module == VK_NULL_HANDLE) {
			throw std::runtime_error(std::string("Manual shader failed to compile.\n") + kernel);
		}

		_reflect(reflection_module);
    }
}