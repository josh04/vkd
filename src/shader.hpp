#pragma once
#include <memory>
#include <vector>
#include <array>
#include <set>
#include "vulkan.hpp"

#include "parameter.hpp"

#include "spirv_reflect.h"
#include "glm/glm.hpp"

#include "vkd_dll.h"

namespace vkd {

    class Device;
    class DescriptorLayout;
    class VKDEXPORT Shader {
    public:
        Shader(std::shared_ptr<Device> device) : _device(device) {}
        ~Shader();
        Shader(Shader&&) = delete;
        Shader(const Shader&) = delete;

        void add(VkShaderStageFlagBits stage, const std::string& path, const std::string& main_name);

        VkPipelineShaderStageCreateInfo * data() { return _stages.data(); }
        size_t size() const { return _stages.size(); }
    private:
        std::shared_ptr<Device> _device = nullptr;
		std::vector<VkPipelineShaderStageCreateInfo> _stages;
        std::vector<std::unique_ptr<std::string>> _main_names; 
    };
    
    class VKDEXPORT ComputeShader {
    public:
        ComputeShader(std::shared_ptr<Device> device) : _device(device) {}
        ~ComputeShader();
        ComputeShader(ComputeShader&&) = delete;
        ComputeShader(const ComputeShader&) = delete;

        void create(const std::string& path, const std::string& main_name);

        VkPipelineShaderStageCreateInfo get() { return _stage; }

        // currently only handling single desc sets
        std::shared_ptr<DescriptorLayout> desc_set_layout() const;
        struct DescriptorCounts {
            uint16_t storage_buffers = 0;
            uint16_t uniform_buffers = 0;
            uint16_t image_samplers = 0;
            uint16_t storage_images = 0;
        };
        DescriptorCounts descriptor_counts() const;
        std::array<int32_t, 3> local_group_sizes() const { return _local_group_sizes; }
        const auto& push_constants() const { return _push_constants; }

        const auto& path() const { return _path; }

        struct ParameterPair {
            ParameterType type;
            std::string name;
            size_t offset;
        };
        const auto& push_constant_types() const { return _types; }

        const auto& binding_names() const { return _set_layouts[0].binding_names; }
        int index_for_name(const std::string& name) const { 
            auto search = _set_layouts[0].binding_names.find(name);
            if (search != _set_layouts[0].binding_names.end()) {
                return search->second;
            } 
            return -1;
        }

    protected:
        void _reflect(SpvReflectShaderModule reflection_module);
        std::shared_ptr<Device> _device = nullptr;
		VkPipelineShaderStageCreateInfo _stage;
        std::string _main_name; 
        std::string _path; 

		struct SetLayoutData {
			uint32_t set_number;
			VkDescriptorSetLayoutCreateInfo create_info;
			std::vector<VkDescriptorSetLayoutBinding> bindings;
            std::map<std::string, int> binding_names;
		};

        std::vector<SetLayoutData> _set_layouts;
        std::array<int32_t, 3> _local_group_sizes;

        std::vector<VkPushConstantRange> _push_constants;

        std::vector<ParameterPair> _types;

        
    };

    class VKDEXPORT ManualComputeShader : public ComputeShader {
    public:
        ManualComputeShader(std::shared_ptr<Device> device) : ComputeShader(device) {}
        ~ManualComputeShader();
        ManualComputeShader(ManualComputeShader&&) = delete;
        ManualComputeShader(const ManualComputeShader&) = delete;

        void create(const std::string& kernel, const std::string& main_name);

    private:
    };
}