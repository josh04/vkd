
#include "OpenColorIO/OpenColorIO.h"
namespace OCIO = OCIO_NAMESPACE;

#include "ocio_static.hpp"
#include "graph_exception.hpp"

namespace vkd {
    const OcioStatic& OcioStatic::GetOCIOConfig() {
        std::scoped_lock lock(s_mutex);
        if (s_ocio == nullptr) {
            s_ocio = std::make_unique<OcioStatic>();
            s_ocio->load();
        }
        return *s_ocio;
    }

    std::mutex OcioStatic::s_mutex;
    std::unique_ptr<OcioStatic> OcioStatic::s_ocio = nullptr;

    void OcioStatic::load() {
        try {
            std::string path = "aces_1.2/config.ocio";

            _config = OCIO::Config::CreateFromFile(path.c_str());

            OCIO::SetCurrentConfig(_config);

            int num_spaces = _config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ACTIVE);
            _space_names.resize(num_spaces);

            for (int i = 0; i < num_spaces; ++i) {
                _space_names[i] = _config->getColorSpaceNameByIndex(i);
            }

        } catch(OCIO::Exception & exception) {
            throw OcioException(std::string("OpenColorIO Error: ") + exception.what());
        }
    }
}