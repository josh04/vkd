#pragma once

#include "../image_uploader.hpp"

namespace vkd {
    struct UploadFormat {
        ImageUploader::InFormat format;
        int32_t width;
        int32_t height;
    };

    static bool operator==(const UploadFormat& lhs, const UploadFormat& rhs) {
        return lhs.format == rhs.format && lhs.width == rhs.width && lhs.height == rhs.height;
    }

    namespace sane {
        enum class SaneChannels {
            Gray,
            RGB,
            Red,
            Green,
            Blue
        };

        struct SaneFormat {
            SaneChannels channels = SaneChannels::RGB;
            //bool last_frame;
            int32_t bytes_per_line = -1;
            int32_t width = -1;
            int32_t height = -1;
            int32_t depth = -1;
        };

        static bool operator!=(const SaneFormat& lhs, const SaneFormat& rhs) {
            return lhs.channels != rhs.channels || lhs.bytes_per_line != rhs.bytes_per_line || lhs.width != rhs.width || lhs.height != rhs.height || lhs.depth != rhs.depth;
        }
    }
}