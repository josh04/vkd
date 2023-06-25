#include "blockedit.hpp"
#include "make_param.hpp"

namespace vkd {

    BlockEditParams::BlockEditParams(ShaderParamMap& map, std::string hash) {
        
        total_frame_count = make_param<ParameterType::p_int>(hash, "_total_frame_count", 0);

        frame_start_block = make_param<ParameterType::p_frame>(hash, "frame_start_block", 0);
        frame_end_block = make_param<ParameterType::p_frame>(hash, "frame_end_block", 0);
        content_relative_start = make_param<ParameterType::p_frame>(hash, "content_relative_start", 0);

        map["_"].emplace(total_frame_count->name(), total_frame_count);
        map["_"].emplace(frame_start_block->name(), frame_start_block);
        map["_"].emplace(frame_end_block->name(), frame_end_block);
        map["_"].emplace(content_relative_start->name(), content_relative_start);
    }
}