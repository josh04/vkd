#pragma once

#include "vulkan.hpp"
#include <fstream>
#include <vector>

#include "spirv_reflect.h"

namespace vkd {

	// Vulkan loads its shaders from an immediate binary representation called SPIR-V
	// Shaders are compiled offline from e.g. GLSL using the reference glslang compiler
	// This function loads such a shader from a binary file and returns a shader module structure

	template<typename T>
	static VkShaderModule load_spirv_shader(VkDevice device, const std::string& filename, const std::vector<T>& shader_code, SpvReflectShaderModule * reflection_module) {
		if (reflection_module) {
			SpvReflectResult result = spvReflectCreateShaderModule(shader_code.size() * sizeof(T), shader_code.data(), reflection_module);
			if (result != SPV_REFLECT_RESULT_SUCCESS) {
				throw std::runtime_error(std::string("Shader failed to reflect: ") + filename + " error code: " + std::to_string(result));
			}
		}

		VkShaderModuleCreateInfo module_create_info{};
		module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		module_create_info.codeSize = shader_code.size() * sizeof(T);
		module_create_info.pCode = (uint32_t*)shader_code.data();

		VkShaderModule shader_module;
		VK_CHECK_RESULT(vkCreateShaderModule(device, &module_create_info, NULL, &shader_module));

		return shader_module;
	}

	static VkShaderModule load_spirv_shader(VkDevice device, const std::string& filename, SpvReflectShaderModule * reflection_module) {
		std::vector<char> shader_code;

		std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

		if (is.is_open()) {
			size_t shader_size = is.tellg();
			is.seekg(0, std::ios::beg);
			// Copy file contents into a buffer
			shader_code.resize(shader_size);
			is.read(shader_code.data(), shader_size);
			is.close();
			assert(shader_size > 0);
		}
        
		if (shader_code.size() > 0) {
			// Create a new shader module that will be used for pipeline creation
			return load_spirv_shader(device, filename, shader_code, reflection_module);
		} else {
			console << "Error: Could not open shader file \"" << filename << "\"" << std::endl;
			return VK_NULL_HANDLE;
		}
	}

}