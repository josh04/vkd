#pragma once

#include <memory>

namespace vulkan {
    class Image;
    class ImageNode {
    public:
        virtual ~ImageNode() = default;

        virtual std::shared_ptr<Image> get_output_image() const = 0;
        virtual float get_output_ratio() const { return 0.0f; }
    };
}