#pragma once
#include <cassert>
#include <iostream>
#include <memory>
#include "vulkan_enum.hpp"
#include <SDL2/SDL.h>
#include "vkd_dll.h"

#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Error: VkResult is \"" << vkd::error_string(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}
#define VK_CHECK_RESULT_TIMEOUT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res == VK_TIMEOUT) {																			\
		std::cout << "Error: VkTimeout at \"" << vkd::error_string(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
	} else if (res != VK_SUCCESS)																		\
	{																									\
		std::cout << "Error: VkResult is \"" << vkd::error_string(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}

namespace vkd {
	class Device;
	class PipelineCache;
	class Renderpass;
	class DrawUI;
	class EngineNode;
	class Fence;
	VKDEXPORT void shutdown();
    void engine_node_init(const std::shared_ptr<EngineNode>& node, const std::string& param_hash_name);
    VKDEXPORT void init(SDL_Window * window, SDL_Renderer * renderer);
	VKDEXPORT void ui(bool& quit);
	VKDEXPORT void draw();
    void submit_buffer(VkQueue queue, VkCommandBuffer buf, Fence * fence);
    void build_command_buffers();

	Device& device();
	std::shared_ptr<vkd::EngineNode> make(const std::string& node_type, const std::string& param_hash_name);

	VKDEXPORT DrawUI& get_ui();
}