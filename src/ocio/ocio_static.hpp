#pragma once

#include <string>
#include <mutex>
#include <memory>
#include <vector>

#include "graph_exception.hpp"

#include "OpenColorIO/OpenColorIO.h"
namespace OCIO = OCIO_NAMESPACE;

namespace vkd {
    class OcioStatic {
    public:
        OcioStatic() = default;
        ~OcioStatic() = default;
        OcioStatic(OcioStatic&&) = delete;
        OcioStatic(const OcioStatic&) = delete;

        void load();

        size_t num_spaces() const { return _space_names.size(); }
        const auto& space_names() const { return _space_names; }
        const std::string& space_name_at_index(size_t i) const { 
            if ( i < _space_names.size() && i >= 0) {
                return _space_names[i]; 
            } else {
                throw OcioException{"Ocio index out of bounds."};
            }
        }
        int32_t index_from_space_name(const std::string& name) const {
            int32_t in = 0;
            for (auto&& str : _space_names) {
                if (str == name) { return in; }
                in++;
            }
            return -1;
        }

        OCIO::ConstConfigRcPtr get() const { return _config; }

        static const OcioStatic& GetOCIOConfig();
    private:
        OCIO::ConstConfigRcPtr _config = nullptr;
        std::vector<std::string> _space_names;

        static std::mutex s_mutex;
        static std::unique_ptr<OcioStatic> s_ocio;
    };
}