#pragma once
        
#include <memory>
#include <string>

namespace enki {
	class TaskSet;
}

namespace vkd {
    class Device;
    class Image;
    enum class ImmediateFormat {
        EXR,
        PNG,
        JPG
    };
    std::string immediate_exr(const std::shared_ptr<Device>& device, std::string filename, ImmediateFormat format, const std::shared_ptr<Image>& image, enki::TaskSet *& task);
}