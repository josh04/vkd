#pragma once

#include "vkd_dll.h"

namespace vkd {
    enum class ApplicationMode {
        Standard,
        Photo
    };

    VKDEXPORT int run(ApplicationMode mode);
    VKDEXPORT ApplicationMode Mode();
}