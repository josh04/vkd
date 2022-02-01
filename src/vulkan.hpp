#pragma once
#include <cassert>
#include <iostream>
#include <sstream>
#include <memory>
#include "vulkan_enum.hpp"
#include "graph_exception.hpp"
#include <SDL2/SDL.h>
#include "vkd_dll.h"


#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult _RES_ = (f);																					\
	if (_RES_ != VK_SUCCESS)																				\
	{																									\
		std::stringstream strm; \
		strm << "Error: VkResult is \"" << vkd::error_string(_RES_) << " (" << _RES_ << ")" << "\" in " << __FILE__ << " at line " << __LINE__; \
		std::cout << strm.str() << std::endl; \
		throw VkException(strm.str()); \
	}																									\
}
#define VK_CHECK_RESULT_NO_THROW(f)																				\
{																										\
	VkResult _RES_ = (f);																					\
	if (_RES_ != VK_SUCCESS)																				\
	{																									\
		std::stringstream strm; \
		strm << "Error: VkResult is \"" << vkd::error_string(_RES_) << " (" << _RES_ << ")" << "\" in " << __FILE__ << " at line " << __LINE__; \
		std::cout << strm.str() << std::endl; \
		assert(_RES == VK_SUCCESS); \
	}																									\
}
#define VK_CHECK_RESULT_TIMEOUT(f)																				\
{																										\
	VkResult _RES_ = (f);																					\
	if (_RES_ != VK_SUCCESS && _RES_ != VK_TIMEOUT)																		\
	{																									\
		std::stringstream strm; \
		strm << "Error: VkResult is \"" << vkd::error_string(_RES_) << " (" << _RES_ << ")" << "\" in " << __FILE__ << " at line " << __LINE__; \
		std::cout << strm.str() << std::endl; \
		throw VkException(strm.str()); \
	}																									\
}

namespace enki {
	class TaskScheduler;
}

namespace vkd {
	class Device;
	class PipelineCache;
	class Renderpass;
	class DrawUI;
	class EngineNode;
	class Fence;
	class Instance;
	VKDEXPORT std::shared_ptr<Instance> createInstance(bool);
	VKDEXPORT void shutdown();
    void engine_node_init(const std::shared_ptr<EngineNode>& node, const std::string& param_hash_name);
    VKDEXPORT void init(SDL_Window * window, SDL_Renderer * renderer);
	VKDEXPORT void ui(bool& quit);
	VKDEXPORT void draw();
    //void submit_buffer(VkQueue queue, VkCommandBuffer buf, Fence * fence);

    enki::TaskScheduler& ts();

	void synchronise_to_host_thread();
    void build_command_buffers();

	Device& device();
	std::shared_ptr<vkd::EngineNode> make(const std::string& node_type, const std::string& param_hash_name);

	VKDEXPORT DrawUI& get_ui();
}