#pragma once

#include <memory>
#include <map>
#include <array>
#include <vector>

#include "vulkan.hpp"
#include "shader.hpp"
#include "compute_pipeline.hpp"

#include "graph_exception.hpp"
#include "vkd_dll.h"

#include <glm/glm.hpp>

namespace vkd {
    class Device;
    class ComputePipeline;
    class Buffer;
    class Image;
    class DescriptorSet;
    class DescriptorPool;
    class DescriptorLayout;
    class ComputeShader;
    class CommandBuffer;
    class EngineNode;

    class VKDEXPORT Kernel {
    public:
        static constexpr std::array<int32_t, 3> default_local_sizes = {16, 16, 1};
        Kernel(std::shared_ptr<Device> device, const std::string& param_hash) : _device(device), _param_hash(param_hash) {}
        ~Kernel() = default;
        Kernel(Kernel&&) = delete;
        Kernel(const Kernel&) = delete;

        static std::shared_ptr<Kernel> make(EngineNode& node, std::string path, std::string func_name, std::array<int32_t, 3> local_sizes);
        static std::shared_ptr<Kernel> make(EngineNode& node, std::unique_ptr<ComputeShader> shader, const std::string& hash_name, std::array<int32_t, 3> local_sizes);

        void init(EngineNode& node, std::string path, std::string func_name, std::array<int32_t, 3> local_sizes = default_local_sizes);
        void init(EngineNode& node, std::unique_ptr<ComputeShader> shader, const std::string& hash_name, std::array<int32_t, 3> local_sizes);
        void init(std::string path, std::string func_name, std::array<int32_t, 3> local_sizes = default_local_sizes);
        void init(std::unique_ptr<ComputeShader> shader, const std::string& hash_name, std::array<int32_t, 3> local_sizes);

        std::shared_ptr<DescriptorPool> make_pool();

        std::string get_push_arg_type(std::string name);
        
        std::shared_ptr<ParameterInterface> get_param_by_name(std::string name) {
            auto search = _params.find(name);
            if (search == _params.end()) {
                return nullptr;
            }
            return search->second;
        }

        template<typename T> 
        bool set_push_arg_by_name(std::string name, T val) {
            auto search = _params.find(name);
            if (search == _params.end()) {
                return false;
            }
            search->second->as<T>().set_force(val);

            return true;
        }

        template<typename T> 
        bool set_push_arg_by_name(VkCommandBuffer buf, std::string name, T val) {
            auto search = _params.find(name);
            if (search == _params.end()) {
                return false;
            }
            search->second->as<T>().set_force(val);

            if (buf != VK_NULL_HANDLE) {
                set_push_arg(buf, *search->second);
            }
            return true;
        }

        void set_push_arg(VkCommandBuffer buf, ParameterInterface& intf) {
            auto size = intf.size();
            auto offset = intf.offset();
            void * data = intf.data();

            if (_push_constant_size == 0) {
                return;
            }

            if (size + offset > _push_constant_size) {
                throw GraphException("Param out of range for push constant");
            }

            if (size == 0) {
                throw std::runtime_error("Push constant size zero.");
            }
            
            vkCmdPushConstants(buf, _full_pipeline.pipeline->layout()->get(), VK_SHADER_STAGE_COMPUTE_BIT, (uint32_t)offset, (uint32_t)size, data);
        }

        void set_arg(int32_t index, std::shared_ptr<Buffer> buffer);
        void set_arg(int32_t index, std::shared_ptr<Image> buffer);

        const auto& arg_names() const { return _shader->binding_names(); }
        int arg_index_for_name(const std::string& name) const { return _shader->index_for_name(name); }

        void set_offset(int32_t x, int32_t y, int32_t z, int32_t w);
        void set_offset(int32_t x, int32_t y, int32_t z) { set_offset(x, y, z, 0); }
        void set_offset(int32_t x, int32_t y) { set_offset(x, y, 0, 0); }
        void set_offset(glm::ivec2 xy) { set_offset(xy.x, xy.y, 0, 0); }

        void update();
        void bind(VkCommandBuffer buf) const;

        const auto& params() const { return _params; }
        const auto& public_params() const { return _public_params; }
        auto name() const { return _shader->path(); }

	    void dispatch(CommandBuffer& cbuf, int32_t global_x, int32_t global_y =1 , int32_t global_z = 1);
	    void dispatch(VkCommandBuffer buf, int32_t global_x, int32_t global_y =1 , int32_t global_z = 1);
    private:
        struct PartialPipeline {
            std::shared_ptr<ComputePipeline> pipeline = nullptr;
            std::array<int32_t, 3> local_sizes = {0,0,0};
        };
        void _create_pipeline(PartialPipeline& pipeline, std::array<int32_t, 3> local_sizes);

        struct ExecutionConstants {
            glm::ivec4 offset;
        };
        void _push_execution_offset(VkCommandBuffer buf, const glm::ivec4& offset);

        ExecutionConstants _constants = {};

        std::shared_ptr<Device> _device = nullptr;
        std::shared_ptr<PipelineCache> _pipeline_cache = nullptr;
        std::unique_ptr<ComputeShader> _shader = nullptr;

        std::string _param_hash = "";

        PartialPipeline _full_pipeline;
        PartialPipeline _overflow_x_pipeline;
        PartialPipeline _overflow_y_pipeline;
        PartialPipeline _overflow_z_pipeline;
        PartialPipeline _overflow_yz_pipeline;
        PartialPipeline _overflow_xy_pipeline;
        PartialPipeline _overflow_xz_pipeline;
        PartialPipeline _overflow_xyz_pipeline;

        void _dispatch_overflow(VkCommandBuffer buf, std::array<int32_t, 3> size, std::array<int32_t, 3> count);

        std::shared_ptr<DescriptorLayout> _desc_set_layout = nullptr;
        std::shared_ptr<DescriptorSet> _desc_set = nullptr;

        struct Arg {
            std::shared_ptr<Buffer> buffer;
            std::shared_ptr<Image> image;
        };

        std::map<int32_t, Arg> _args;

        std::array<int32_t, 3> _local_group_sizes;

        bool _args_changed = true;

        size_t _push_constant_size = 0;
        
        std::map<std::string, std::shared_ptr<ParameterInterface>> _params;
        std::map<std::string, std::shared_ptr<ParameterInterface>> _public_params;
    };
}