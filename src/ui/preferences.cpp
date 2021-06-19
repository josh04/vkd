#include "preferences.hpp"

#include "platform_folders.h"

#include "ghc/filesystem.hpp"
#include <iostream>

#include "cereal/cereal.hpp"

CEREAL_CLASS_VERSION(vkd::Preferences, 0);

namespace {
    std::string vkd_folder = "/vkd";
}

namespace vkd {    
    std::string Preferences::Path() {
        ghc::filesystem::create_directory(sago::getConfigHome() + vkd_folder);
        return sago::getConfigHome() + vkd_folder+ "/settings.json";
    }
}