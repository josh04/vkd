#include "particles.hpp"
#include <random>
#include "command_buffer.hpp"
#include "device.hpp"
#include "pipeline.hpp"
#include "shader.hpp"
#include "vulkan.hpp"

namespace vkd {
    REGISTER_NODE("particles", "particles", Particles);
    void Particles::init() {
        _desc_pool = std::make_shared<DescriptorPool>(_device);
        _desc_pool->add_uniform_buffer(1);
        _desc_pool->add_storage_buffer(1);
        _desc_pool->create(1); // one for gfx, one for compute

        _compute_command_buffer = create_command_buffer(_device->logical_device(), _device->command_pool());
        
		std::default_random_engine engine(0);// : (unsigned)time(nullptr));
		std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

		// Initial particle positions
		std::vector<Particle> particle_buffer(PARTICLE_COUNT);
		for (auto& particle : particle_buffer) {
			particle.pos = glm::vec2(dist(engine), dist(engine));
			particle.vel = glm::vec2(0.0f);
			particle.gradientPos.x = particle.pos.x / 2.0f;
		}

		VkDeviceSize storage_buffer_size = particle_buffer.size() * sizeof(Particle);

        _compute_uniform_buffer = std::make_shared<UniformBuffer>(_device);
        _compute_uniform_buffer->create(sizeof(ComputeUBO));
        _mapped_uniform_buffer = (uint8_t *)_compute_uniform_buffer->map();
        memcpy(_mapped_uniform_buffer, &ubo, sizeof(ubo));
        //_compute_uniform_buffer->unmap();

        _compute_storage_buffer = std::make_shared<Buffer>(_device);
        _compute_storage_buffer->init(storage_buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        _compute_storage_buffer->stage({{(void *)particle_buffer.data(), particle_buffer.size()}});

        auto pipeline_cache = std::make_shared<PipelineCache>(_device);
        pipeline_cache->create();
        _compute_pipeline = std::make_shared<ComputePipeline>(_device, pipeline_cache);

        _compute_desc_set_layout = std::make_shared<DescriptorLayout>(_device);
        _compute_desc_set_layout->add(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);
        _compute_desc_set_layout->add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);
        _compute_desc_set_layout->create();

        _compute_desc_set = std::make_shared<DescriptorSet>(_device, _compute_desc_set_layout, _desc_pool);
        _compute_desc_set->add_buffer(*_compute_storage_buffer);
        _compute_desc_set->add_buffer(*_compute_uniform_buffer);
        _compute_desc_set->create();
        
		auto compute_shader = std::make_unique<ComputeShader>(_device);
        compute_shader->create("shaders/compute/particle.comp.spv", "main");
        _compute_pipeline->create(_compute_desc_set_layout->get(), compute_shader.get());

        begin_command_buffer(_compute_command_buffer);
        _compute_pipeline->bind(_compute_command_buffer, _compute_desc_set->get());
		vkCmdDispatch(_compute_command_buffer, PARTICLE_COUNT / 256, 1, 1);
        end_command_buffer(_compute_command_buffer);

        _compute_complete = Semaphore::make(_device);
    }

    void Particles::post_init()
    {
    }

    void Particles::execute(ExecutionType type, const SemaphorePtr& wait_semaphore, Fence * fence) {
		// Wait for rendering finished
		VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

		// Submit compute commands
		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &_compute_command_buffer;
		submit_info.waitSemaphoreCount = (wait_semaphore != VK_NULL_HANDLE) ? 1 : 0;
		submit_info.pWaitSemaphores = (wait_semaphore != VK_NULL_HANDLE) ? &wait_semaphore->get() : nullptr;
		submit_info.pWaitDstStageMask = &wait_stage_mask;
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &_compute_complete->get();
		{
			std::lock_guard<std::mutex> lock(_device->queue_mutex());
			VK_CHECK_RESULT(vkQueueSubmit(_device->compute_queue(), 1, &submit_info, VK_NULL_HANDLE));
		}

	    static float timer = 0.0f;
	    static float animStart = 20.0f;
        float frame_timer = 0.01666f;

		//if (!attachToCursor)
		//{
			if (animStart > 0.0f) {
				animStart -= frame_timer * 5.0f;
			}
			else if (animStart <= 0.0f) {
				timer += frame_timer * 0.04f;
				if (timer > 1.f) {
					timer = 0.f;
                }
			}
		//}

		ubo.deltaT = frame_timer * 2.5f;
		//if (!attachToCursor) {
			ubo.destX = sin(glm::radians(timer * 360.0f)) * 0.75f;
			ubo.destY = 0.0f;
		/*} else {
			float normalizedMx = (mousePos.x - static_cast<float>(width / 2)) / static_cast<float>(width / 2);
			float normalizedMy = (mousePos.y - static_cast<float>(height / 2)) / static_cast<float>(height / 2);
			ubo.destX = normalizedMx;
			ubo.destY = normalizedMy;
		}*/

		memcpy(_mapped_uniform_buffer, &ubo, sizeof(ComputeUBO));
    }
}