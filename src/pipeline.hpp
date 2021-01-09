#pragma once

#include <array>
#include <vector>
#include <memory>
#include <fstream>
#include "vulkan.hpp"

namespace vulkan {
	class Device; 
	class Renderpass; 
	class Shader;
	class VertexInput;
    struct Vertex {
        float position[3];
        float color[3];
    };

	class PipelineLayout {
	public:
		PipelineLayout(std::shared_ptr<Device> device) : _device(device) {}
		virtual ~PipelineLayout() = default;
		PipelineLayout(PipelineLayout&&) = delete;
		PipelineLayout(const PipelineLayout&) = delete;

		virtual void constant(VkPipelineLayoutCreateInfo&) {}

		void create(VkDescriptorSetLayout desc_set_layout); 
		void bind(VkCommandBuffer buf, VkDescriptorSet desc_set, VkPipelineBindPoint bind_point) const;
		auto get() const { return _layout; }

		void add_constant(VkPushConstantRange range);
		void add_constant(VkShaderStageFlags stage_flags, size_t offset, size_t size);
	protected:
		std::shared_ptr<Device> _device = nullptr;
		VkPipelineLayout _layout;

		std::vector<VkPushConstantRange> _push_constant_ranges;
	};

	class PipelineCache {
	public:
		PipelineCache(std::shared_ptr<Device> device) : _device(device) {}
		~PipelineCache() = default;
		PipelineCache(PipelineCache&&) = delete;
		PipelineCache(const PipelineCache&) = delete;

		void create();

		auto get() const { return _cache; }
	private:
		std::shared_ptr<Device> _device = nullptr;
		VkPipelineCache _cache;
	};

	class Pipeline {
	public:
		Pipeline(std::shared_ptr<Device> device, 
				 std::shared_ptr<PipelineCache> pipeline_cache) 
				 : _device(device), _cache(pipeline_cache) { _layout = std::make_shared<PipelineLayout>(device); }
		virtual ~Pipeline() = default;
		Pipeline(Pipeline&&) = delete;
		Pipeline(const Pipeline&) = delete;

		virtual void bind(VkCommandBuffer buf, VkDescriptorSet desc_set) const;
		auto get() const { return _pipeline; }
		auto cache() const { return _cache; }
		auto layout() const { return _layout; }

		void set_topology(VkPrimitiveTopology topology) { _topology = topology; }
		void set_depth_test(bool set) { _depth_test = set; }
	protected:
		std::shared_ptr<Device> _device = nullptr;
		std::shared_ptr<PipelineCache> _cache = nullptr;
		std::shared_ptr<PipelineLayout> _layout = nullptr;
		VkPipeline _pipeline;

		VkPipelineCreateFlags _pipeline_create_flags = 0;
		VkPipelineBindPoint _bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
		VkPrimitiveTopology _topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		bool _depth_test = VK_TRUE;
	};

	class GraphicsPipeline : public Pipeline {
	public:
		GraphicsPipeline(std::shared_ptr<Device> device, 
						 std::shared_ptr<PipelineCache> pipeline_cache, 
						 std::shared_ptr<Renderpass> renderpass) : Pipeline(device, pipeline_cache), _renderpass(renderpass) {}
        ~GraphicsPipeline() = default;
		GraphicsPipeline(GraphicsPipeline&&) = delete;
		GraphicsPipeline(const GraphicsPipeline&) = delete;

		void create(VkDescriptorSetLayout desc_set_layout, std::unique_ptr<Shader> shader_stages, std::unique_ptr<VertexInput> vertex_input);
		void create(std::shared_ptr<PipelineLayout> layout, std::unique_ptr<Shader> shader_stages, std::unique_ptr<VertexInput> vertex_input);
	protected:
		std::vector<VkPipelineColorBlendAttachmentState> _blend_attachment_states;
		virtual void _blend_attachments();

		std::shared_ptr<Renderpass> _renderpass = nullptr;
	};

}
