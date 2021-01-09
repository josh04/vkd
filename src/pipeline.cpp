#include <array>
#include <vector>
#include "vulkan.hpp"
#include "spirv.hpp"
#include "device.hpp"
#include "pipeline.hpp"
#include "renderpass.hpp"
#include "shader.hpp"
#include "vertex_input.hpp"

namespace vulkan {
    void PipelineLayout::create(VkDescriptorSetLayout desc_set_layout) {
        // Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
		// In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
		VkPipelineLayoutCreateInfo pipe_layout_create_info = {};
		pipe_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipe_layout_create_info.pNext = nullptr;
		pipe_layout_create_info.setLayoutCount = 1;
		pipe_layout_create_info.pSetLayouts = &desc_set_layout;

		constant(pipe_layout_create_info);

		pipe_layout_create_info.pushConstantRangeCount = _push_constant_ranges.size();
		pipe_layout_create_info.pPushConstantRanges = _push_constant_ranges.data();

		VK_CHECK_RESULT(vkCreatePipelineLayout(_device->logical_device(), &pipe_layout_create_info, nullptr, &_layout));
    }

	void PipelineLayout::add_constant(VkPushConstantRange range) {
		_push_constant_ranges.push_back(range);
	}

	void PipelineLayout::add_constant(VkShaderStageFlags stage_flags, size_t offset, size_t size) {
		VkPushConstantRange push_constant_range = {};

		push_constant_range.stageFlags = stage_flags;
		push_constant_range.offset = offset;
		push_constant_range.size = size;

		add_constant(push_constant_range);
	}

	void PipelineLayout::bind(VkCommandBuffer buf, VkDescriptorSet desc_set, VkPipelineBindPoint bind_point) const {
        // Bind descriptor sets describing shader binding points
        vkCmdBindDescriptorSets(buf, bind_point, _layout, 0, 1, &desc_set, 0, nullptr);
	}

    void PipelineCache::create() {
        VkPipelineCacheCreateInfo pipeline_cache_create_info = {};
        memset(&pipeline_cache_create_info, 0, sizeof(VkPipelineCacheCreateInfo));
        pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        VK_CHECK_RESULT(vkCreatePipelineCache(_device->logical_device(), &pipeline_cache_create_info, nullptr, &_cache));
    }

    void Pipeline::bind(VkCommandBuffer buf, VkDescriptorSet desc_set) const {
		_layout->bind(buf, desc_set, _bind_point);
        // Bind the rendering pipeline
        // The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
        vkCmdBindPipeline(buf, _bind_point, _pipeline);
    }

	void GraphicsPipeline::create(VkDescriptorSetLayout desc_set_layout, std::unique_ptr<Shader> shader_stages, std::unique_ptr<VertexInput> vertex_input) {
        _layout->create(desc_set_layout);
        create(_layout, std::move(shader_stages), std::move(vertex_input));
    }

	void GraphicsPipeline::_blend_attachments() {
		VkPipelineColorBlendAttachmentState blend_attachment_state = {};
		blend_attachment_state.blendEnable = VK_FALSE;
		blend_attachment_state.colorWriteMask = 0xF;

		_blend_attachment_states.push_back(blend_attachment_state);
	}

	void GraphicsPipeline::create(std::shared_ptr<PipelineLayout> layout, std::unique_ptr<Shader> shader_stages, std::unique_ptr<VertexInput> vertex_input) {
        _layout = layout;

		// Create the graphics pipeline used in this example
		// Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
		// A pipeline is then stored and hashed on the GPU making pipeline changes very fast
		// Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

		VkGraphicsPipelineCreateInfo pipeline_create_info = {};

		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
		pipeline_create_info.layout = _layout->get();
		// Renderpass this pipeline is attached to
		pipeline_create_info.renderPass = _renderpass->get();
		pipeline_create_info.flags = _pipeline_create_flags;

		// Construct the different states making up the pipeline

		// Input assembly state describes how primitives are assembled
		// This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = _topology;

		// Rasterization state
		VkPipelineRasterizationStateCreateInfo rasterizationState = {};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.depthClampEnable = VK_FALSE;
		rasterizationState.rasterizerDiscardEnable = VK_FALSE;
		rasterizationState.depthBiasEnable = VK_FALSE;
		rasterizationState.lineWidth = 1.0f;

		// Color blend state describes how blend factors are calculated (if used)
		// We need one blend attachment state per color attachment (even if blending is not used)
		_blend_attachments();
		VkPipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = _blend_attachment_states.size();
		colorBlendState.pAttachments = _blend_attachment_states.data();

		// Viewport state sets the number of viewports and scissor used in this pipeline
		// Note: This is actually overridden by the dynamic states (see below)
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// Enable dynamic states
		// Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
		// To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
		// For this example we will set the viewport and scissor using dynamic states
		std::vector<VkDynamicState> dynamicStateEnables;
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pDynamicStates = dynamicStateEnables.data();
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

		// Depth and stencil state containing depth and stencil compare and test operations
		// We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
		VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = _depth_test;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
		depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
		depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
		depthStencilState.stencilTestEnable = VK_FALSE;
		depthStencilState.front = depthStencilState.back;

		// Multi sampling state
		// This example does not make use of multi sampling (for anti-aliasing), the state must still be set and passed to the pipeline
		VkPipelineMultisampleStateCreateInfo multisampleState = {};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.pSampleMask = nullptr;

		// Vertex input state used for pipeline creation
		VkPipelineVertexInputStateCreateInfo vertexInputState = {};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.vertexBindingDescriptionCount = vertex_input->get_binding_size();
		vertexInputState.pVertexBindingDescriptions = vertex_input->get_binding_data();
		vertexInputState.vertexAttributeDescriptionCount = vertex_input->get_attribute_size();
		vertexInputState.pVertexAttributeDescriptions = vertex_input->get_attribute_data();

		// Set pipeline shader stage info
		pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages->size());
		pipeline_create_info.pStages = shader_stages->data();

		// Assign the pipeline states to the pipeline creation info structure
		pipeline_create_info.pVertexInputState = &vertexInputState;
		pipeline_create_info.pInputAssemblyState = &inputAssemblyState;
		pipeline_create_info.pRasterizationState = &rasterizationState;
		pipeline_create_info.pColorBlendState = &colorBlendState;
		pipeline_create_info.pMultisampleState = &multisampleState;
		pipeline_create_info.pViewportState = &viewportState;
		pipeline_create_info.pDepthStencilState = &depthStencilState;
		pipeline_create_info.pDynamicState = &dynamicState;

		// Create rendering pipeline using the specified states
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(_device->logical_device(), _cache->get(), 1, &pipeline_create_info, nullptr, &_pipeline));
	}

}
