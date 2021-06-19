#include "vulkan_enum.hpp"

namespace vkd {

    std::string error_string(VkResult errorCode)
    {
        switch (errorCode)
        {
#define STR(r) case VK_ ##r: return #r
            STR(NOT_READY);
            STR(TIMEOUT);
            STR(EVENT_SET);
            STR(EVENT_RESET);
            STR(INCOMPLETE);
            STR(ERROR_OUT_OF_HOST_MEMORY);
            STR(ERROR_OUT_OF_DEVICE_MEMORY);
            STR(ERROR_INITIALIZATION_FAILED);
            STR(ERROR_DEVICE_LOST);
            STR(ERROR_MEMORY_MAP_FAILED);
            STR(ERROR_LAYER_NOT_PRESENT);
            STR(ERROR_EXTENSION_NOT_PRESENT);
            STR(ERROR_FEATURE_NOT_PRESENT);
            STR(ERROR_INCOMPATIBLE_DRIVER);
            STR(ERROR_TOO_MANY_OBJECTS);
            STR(ERROR_FORMAT_NOT_SUPPORTED);
            STR(ERROR_SURFACE_LOST_KHR);
            STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
            STR(SUBOPTIMAL_KHR);
            STR(ERROR_OUT_OF_DATE_KHR);
            STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
            STR(ERROR_VALIDATION_FAILED_EXT);
            STR(ERROR_INVALID_SHADER_NV);
#undef STR
        default:
            return "UNKNOWN_ERROR";
        }
    }

    std::string physical_device_to_string(VkPhysicalDeviceType type)
    {
        switch (type)
        {
#define STR(r) case VK_PHYSICAL_DEVICE_TYPE_ ##r: return #r
            STR(OTHER);
            STR(INTEGRATED_GPU);
            STR(DISCRETE_GPU);
            STR(VIRTUAL_GPU);
#undef STR
        default: return "UNKNOWN_DEVICE_TYPE";
        }
    }

#define PHYS_FEAT(NAME) outs += std::string(#NAME": ") + (feats.NAME ? "true" : "false") + "\n"; 

    std::string physical_device_8bit_features_string(const VkPhysicalDevice8BitStorageFeaturesKHR& feats) {
        std::string outs = "";
        
        PHYS_FEAT(storageBuffer8BitAccess);
        PHYS_FEAT(uniformAndStorageBuffer8BitAccess);
        PHYS_FEAT(storagePushConstant8);

        return outs;
    }

    std::string physical_device_16bit_features_string(const VkPhysicalDevice16BitStorageFeatures& feats) {
        std::string outs = "";
        
        PHYS_FEAT(storageBuffer16BitAccess);
        PHYS_FEAT(uniformAndStorageBuffer16BitAccess);
        PHYS_FEAT(storagePushConstant16);
        PHYS_FEAT(storageInputOutput16);

        return outs;
    }

    std::string physical_device_features_string(const VkPhysicalDeviceFeatures& feats) {
        std::string outs = "";
        
        PHYS_FEAT(robustBufferAccess);
        PHYS_FEAT(fullDrawIndexUint32);
        PHYS_FEAT(imageCubeArray);
        PHYS_FEAT(independentBlend);
        PHYS_FEAT(geometryShader);
        PHYS_FEAT(tessellationShader);
        PHYS_FEAT(sampleRateShading);
        PHYS_FEAT(dualSrcBlend);
        PHYS_FEAT(logicOp);
        PHYS_FEAT(multiDrawIndirect);
        PHYS_FEAT(drawIndirectFirstInstance);
        PHYS_FEAT(depthClamp);
        PHYS_FEAT(depthBiasClamp);
        PHYS_FEAT(fillModeNonSolid);
        PHYS_FEAT(depthBounds);
        PHYS_FEAT(wideLines);
        PHYS_FEAT(largePoints);
        PHYS_FEAT(alphaToOne);
        PHYS_FEAT(multiViewport);
        PHYS_FEAT(samplerAnisotropy);
        PHYS_FEAT(textureCompressionETC2);
        PHYS_FEAT(textureCompressionASTC_LDR);
        PHYS_FEAT(textureCompressionBC);
        PHYS_FEAT(occlusionQueryPrecise);
        PHYS_FEAT(pipelineStatisticsQuery);
        PHYS_FEAT(vertexPipelineStoresAndAtomics);
        PHYS_FEAT(fragmentStoresAndAtomics);
        PHYS_FEAT(shaderTessellationAndGeometryPointSize);
        PHYS_FEAT(shaderImageGatherExtended);
        PHYS_FEAT(shaderStorageImageExtendedFormats);
        PHYS_FEAT(shaderStorageImageMultisample);
        PHYS_FEAT(shaderStorageImageReadWithoutFormat);
        PHYS_FEAT(shaderStorageImageWriteWithoutFormat);
        PHYS_FEAT(shaderUniformBufferArrayDynamicIndexing);
        PHYS_FEAT(shaderSampledImageArrayDynamicIndexing);
        PHYS_FEAT(shaderStorageBufferArrayDynamicIndexing);
        PHYS_FEAT(shaderStorageImageArrayDynamicIndexing);
        PHYS_FEAT(shaderClipDistance);
        PHYS_FEAT(shaderCullDistance);
        PHYS_FEAT(shaderFloat64);
        PHYS_FEAT(shaderInt64);
        PHYS_FEAT(shaderInt16);
        PHYS_FEAT(shaderResourceResidency);
        PHYS_FEAT(shaderResourceMinLod);
        PHYS_FEAT(sparseBinding);
        PHYS_FEAT(sparseResidencyBuffer);
        PHYS_FEAT(sparseResidencyImage2D);
        PHYS_FEAT(sparseResidencyImage3D);
        PHYS_FEAT(sparseResidency2Samples);
        PHYS_FEAT(sparseResidency4Samples);
        PHYS_FEAT(sparseResidency8Samples);
        PHYS_FEAT(sparseResidency16Samples);
        PHYS_FEAT(sparseResidencyAliased);
        PHYS_FEAT(variableMultisampleRate);
        PHYS_FEAT(inheritedQueries);

        #undef PHYS_FEAT
        return outs;
    }

    std::string memory_property_flags_to_string(VkMemoryPropertyFlags flags) {
        std::string outs = "";
        #define FLAG(NAME) outs += (flags & NAME) ? "\t"#NAME"\n" : "";

        FLAG(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        FLAG(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        FLAG(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        FLAG(VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
        FLAG(VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
        FLAG(VK_MEMORY_PROPERTY_PROTECTED_BIT);

        #undef FLAG
        return outs;
    }

    std::string memory_heap_flags_to_string(VkMemoryHeapFlags flags) {
        std::string outs = "";
        #define FLAG(NAME) outs += (flags & NAME) ? "\t"#NAME"\n" : "";

        FLAG(VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
        FLAG(VK_MEMORY_HEAP_MULTI_INSTANCE_BIT);
        
        #undef FLAG
        return outs;
    }

    std::string queue_flags_to_string(VkQueueFlags flags) {
        std::string outs = "";
        #define FLAG(NAME) outs += (flags & NAME) ? "\t"#NAME"\n" : "";

        FLAG(VK_QUEUE_GRAPHICS_BIT);
        FLAG(VK_QUEUE_COMPUTE_BIT);
        FLAG(VK_QUEUE_TRANSFER_BIT);
        FLAG(VK_QUEUE_SPARSE_BINDING_BIT);
        FLAG(VK_QUEUE_PROTECTED_BIT);
        
        #undef FLAG
        return outs;
    }

    std::string format_to_string(VkFormat format) {
        std::string form = "";
        switch (format) {
            // depth formats
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                form = "VK_FORMAT_D32_SFLOAT_S8_UINT";
                break;
            case VK_FORMAT_D32_SFLOAT:
                form = "VK_FORMAT_D32_SFLOAT";
                break;
            case VK_FORMAT_D24_UNORM_S8_UINT:
                form = "VK_FORMAT_D24_UNORM_S8_UINT";
                break;
            case VK_FORMAT_D16_UNORM_S8_UINT:
                form = "VK_FORMAT_D16_UNORM_S8_UINT";
                break;
            case VK_FORMAT_D16_UNORM:
                form = "VK_FORMAT_D16_UNORM";
                break;
            // colour formats
            case VK_FORMAT_B8G8R8A8_UNORM:
                form = "VK_FORMAT_B8G8R8A8_UNORM";
                break;
        }
        return form;
    }
}