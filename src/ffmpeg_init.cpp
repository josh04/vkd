#include "ffmpeg_init.hpp"

#include <mutex>

extern "C" {
#include <libavformat/avformat.h>
}

#include "console.hpp"

namespace vkd {
    void ffmpeg_static_init() {
        static std::once_flag initFlag;
        std::call_once(initFlag, []() {
            av_register_all();
            avformat_network_init();
            av_log_set_callback([](void * ptr, int level, const char* szFmt, va_list varg)
                { 
                    if (level < av_log_get_level()) { 
                        int pre = 1; 
                        char line[1024]; 
                        av_log_format_line(ptr, level, szFmt, varg, line, 1024, &pre); 
                        console << (std::string(line)) << std::endl;; 
                    }
                }
            );
        });
    }
}