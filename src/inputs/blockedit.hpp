#pragma once
#include <algorithm>
#include "engine_node.hpp"

namespace vkd {
    struct BlockEditParams {
        BlockEditParams() {}
        BlockEditParams(ShaderParamMap& map, std::string hash);

        Frame translate(const Frame& frame, bool clamp) {
            auto fin_frame = frame;
            Frame crs = content_relative_start->as<Frame>().get();
            int64_t tot = total_frame_count->as<int>().get();

            if (clamp) {
                fin_frame.index = std::clamp(frame.index + crs.index, (int64_t)0, tot);
            } else {
                fin_frame.index = std::max(frame.index + crs.index, (int64_t)0);
            }
            return fin_frame;
        }
        std::shared_ptr<ParameterInterface> total_frame_count = nullptr;
        std::shared_ptr<ParameterInterface> frame_start_block = nullptr;
        std::shared_ptr<ParameterInterface> frame_end_block = nullptr;
        std::shared_ptr<ParameterInterface> content_relative_start = nullptr;
    };
}
