#include <iostream>
#include "console.hpp"
#include "renderdoc_integration.hpp"

#include "renderdoc_app.h"

namespace vkd {
    namespace {
	    RENDERDOC_API_1_1_2 * rdoc_api = nullptr;

        bool _capture = false;
        bool _started = false;

        VkInstance _instance;
#ifdef _WIN32
        HWND _hwnd;
#endif
    }

#ifdef _WIN32
    void renderdoc_init(VkInstance instance, HWND window) {
        if (HMODULE mod = GetModuleHandleA("renderdoc.dll")) {
            pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
            int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&rdoc_api);

            if (ret != 1) {
                console << "RenderDoc DLL not loaded, graph profiling will be unavailable." << std::endl;
                rdoc_api = nullptr;
                return;
            }

            _instance = instance;
            _hwnd = window;

            console << "RenderDoc DLL loaded." << std::endl;

            rdoc_api->SetActiveWindow(RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(instance), window);
        } else {
            console << "RenderDoc DLL not detected." << std::endl;
        }
    }
#endif
    
    bool renderdoc_enabled() {
        return rdoc_api != nullptr;
    }

    void renderdoc_capture() {
        if (renderdoc_enabled()) {
            console << "Triggering RenderDoc capture." << std::endl;
            rdoc_api->TriggerCapture(); 
            _capture = true;
        }
    }
    
    void renderdoc_start_frame() {
        if (renderdoc_enabled() && _capture) {
            //console << "Starting..." << std::endl;
            //rdoc_api->StartFrameCapture(_instance, _hwnd);  
            _capture = false;
            _started = true;
        }
    }

    void renderdoc_end_frame() {
        if (renderdoc_enabled() && _started) {
            //uint32_t ret = rdoc_api->EndFrameCapture(_instance, _hwnd);  
            //console << "...finished. Status: " << ret << std::endl;
            _started = false;
        }
    }

}