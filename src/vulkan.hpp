#pragma once
#include <cassert>
#include <iostream>
#include <memory>
#include "vulkan_enum.hpp"
#include <SDL2/SDL.h>

#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
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
	void shutdown();
    void engine_node_init(const std::shared_ptr<EngineNode>& node, const std::string& param_hash_name);
    void init(SDL_Window * window, SDL_Renderer * renderer);
	void ui();
	void draw();
    void submit_buffer(VkQueue queue, VkCommandBuffer buf, VkFence fence);
    void build_command_buffers();

	Device& device();
	std::shared_ptr<vkd::EngineNode> make(const std::string& node_type, const std::string& param_hash_name);

	DrawUI& get_ui();
}