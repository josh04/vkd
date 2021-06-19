#include <mutex>
#include "ffmpeg.hpp"
#include "image.hpp"
#include "command_buffer.hpp"
#include "buffer.hpp"
#include "compute/kernel.hpp"
#include "imgui/imgui.h"

extern "C" {
#include <libavformat/avformat.h>
}

namespace vkd {
    REGISTER_NODE("ffmpeg", "ffmpeg", Ffmpeg);

    Ffmpeg::BlockEditParams::BlockEditParams(ShaderParamMap& map) {
        
        frame_start_block = make_param<ParameterType::p_frame>("", "frame_start_block", 0);
        frame_end_block = make_param<ParameterType::p_frame>("", "frame_end_block", 0);
        content_relative_start = make_param<ParameterType::p_frame>("", "content_relative_start", 0);

        map["_"].emplace(frame_start_block->name(), frame_start_block);
        map["_"].emplace(frame_end_block->name(), frame_end_block);
        map["_"].emplace(content_relative_start->name(), content_relative_start);
    }

    Ffmpeg::Ffmpeg() : _block(_params) {
        _path_param = make_param<ParameterType::p_string>("", "path", 0);
        _path_param->as<std::string>().set_default("test.mp4");
        _path_param->tag("filepath");

        _frame_param = make_param<ParameterType::p_frame>("", "frame", 0);
        
        _params["_"].emplace(_path_param->name(), _path_param);
        _params["_"].emplace(_frame_param->name(), _frame_param);

    }

    Ffmpeg::~Ffmpeg() {
        avcodec_close(_codec_context);
        av_free(_codec_context);
        avformat_free_context(_format_context);
    }

    void Ffmpeg::init() {
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
                        std::cout << (std::string(line)) << std::endl;; 
                    }
                }
            );
        });

        _compute_command_buffer = create_command_buffer(_device->logical_device(), _device->command_pool());
        _compute_complete = create_semaphore(_device->logical_device());

        _format_context = avformat_alloc_context();
        
        if (avformat_open_input(&_format_context, _path_param->as<std::string>().get().c_str(), nullptr, nullptr) != 0) {
            throw std::runtime_error("Error while calling avformat_open_input (probably invalid file format)");
        }
        
        if (avformat_find_stream_info(_format_context, nullptr) < 0) {
            throw std::runtime_error("Error while calling avformat_find_stream_info");
        }
        
        _video_stream = nullptr;
        for (unsigned int i = 0; i < _format_context->nb_streams; ++i) {
            auto istream = _format_context->streams[i];		// pointer to a structure describing the stream
            auto codecType = istream->codecpar->codec_type;	// the type of data in this stream, notable values are AVMEDIA_TYPE_VIDEO and AVMEDIA_TYPE_AUDIO
            if (codecType == AVMEDIA_TYPE_VIDEO) {
                _video_stream = istream;
                break;
            }
        }
        
        if (!_video_stream) {
            throw std::runtime_error("Didn't find video streams in the file (probably audio file)");
        }
        
        // getting the required codec structure
        const auto codecL = avcodec_find_decoder(_video_stream->codecpar->codec_id);
        if (codecL == nullptr) {
            throw std::runtime_error("Codec required by video file not available");
        }
        
        // allocating a structure
        _codec_context = avcodec_alloc_context3(codecL);
        
        _codec_context->extradata = _video_stream->codecpar->extradata;
        _codec_context->extradata_size = _video_stream->codecpar->extradata_size;
        
        // initializing the structure by opening the codec
        if (avcodec_open2(_codec_context, codecL, nullptr) < 0) {
            throw std::runtime_error("Could not open codec");
        }
        
        _width = _video_stream->codecpar->width;
        _height = _video_stream->codecpar->height;
        
        _frame_count = _video_stream->nb_frames + 1 - ((_video_stream->nb_frames + 1) % 2);
        
        size_t buffer_size = _width * _height * 1.5 * sizeof(uint8_t);

        _staging_buffer = std::make_shared<StagingBuffer>(_device);
        _staging_buffer->create(buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        _staging_buffer_ptr = (uint32_t*)_staging_buffer->map();

        _gpu_buffer = std::make_shared<StorageBuffer>(_device);
        _gpu_buffer->create(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        _image = std::make_shared<vkd::Image>(_device);
        _image->create_image(VK_FORMAT_R32G32B32A32_SFLOAT, _width, _height, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT );
        _image->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        _image->create_view(VK_IMAGE_ASPECT_COLOR_BIT);

        auto buf = vkd::begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());

        _image->set_layout(buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        vkd::flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);

        _yuv420 = std::make_shared<Kernel>(_device, param_hash_name());
        _yuv420->init("shaders/compute/yuv420.comp.spv", "main", {16, 16, 1});
        register_params(*_yuv420);
        _yuv420->set_arg(0, _gpu_buffer);
        _yuv420->set_arg(1, _image);
        _yuv420->update();

        begin_command_buffer(_compute_command_buffer);

        _gpu_buffer->copy(*_staging_buffer, buffer_size, _compute_command_buffer);
        _gpu_buffer->barrier(_compute_command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        _yuv420->dispatch(_compute_command_buffer, _width, _height);
        
        end_command_buffer(_compute_command_buffer);

        //_local_timeline->frame_count = _frame_count;

        //_local_timeline->name = _path_param->as<std::string>().get().c_str();

        _current_frame = 0;
    }

    bool Ffmpeg::update() {
        bool updated = false;

        scrub(_frame_param->as<Frame>().get());

        if (_force_scrub) {
            //scrub(_timeline->current_frame());
            //_local_timeline->frame_jumped = false;
            _force_scrub = false;
            updated = true;
        }

        // Wait for new frame
        if (_decode_next_frame) {
            std::shared_ptr<AVFrame> avFrame(av_frame_alloc(), [](AVFrame* a){ av_frame_free(&a); });

            bool got_frame = false;
            std::shared_ptr<AVPacket> packet(av_packet_alloc(), [](AVPacket* a){ av_packet_free(&a); });     
            while (!got_frame) {
                int ret = avcodec_receive_frame(_codec_context, avFrame.get());
                if (ret == AVERROR(EAGAIN)) {
                    if (av_read_frame(_format_context, packet.get()) < 0) {
                        _eof = true;
                        break;
                        //throw std::runtime_error("ran out of packets");
                    }
                    if (packet->stream_index == _video_stream->index) {
                        avcodec_send_packet(_codec_context, packet.get());
                    }
                } else if (ret == AVERROR_EOF) {
                    return false;
                } else if (ret == AVERROR(EINVAL)) {
                    throw std::runtime_error("decode failed");
                } else if (avFrame->pkt_pts < _target_pts) {
                    //std::cout << "target pts: " << _target_pts << "pts: " << packet->pts << " dts: " << packet->pts << std::endl;
                    //std::cout << "skippe frame: " << avFrame->pkt_pts << std::endl;
                    continue;
                } else if (ret == 0) {
                    got_frame = true;
                }
            }

            //std::cout << "pts: " << packet->pts << " dts: " << packet->pts << std::endl;
            //std::cout << "coded frame number: " << avFrame->coded_picture_number << std::endl;

            // do something with frame

            //std::cout << "got frame:" << _video_stream->codecpar->format << std::endl;
            //std::cout << "avframe:" << _codec_context->pix_fmt << std::endl;

            if (!_eof) {
                for (int j = 0; j < _height; ++j) {
                    memcpy((uint8_t*)_staging_buffer_ptr + j * _width, avFrame->data[0] + j * avFrame->linesize[0], _width);
                    if (j < _height / 2) {
                        memcpy((uint8_t*)_staging_buffer_ptr + _width * _height       + j * _width/2,   avFrame->data[1] + j * avFrame->linesize[1], _width/2);
                        memcpy((uint8_t*)_staging_buffer_ptr + _width * _height * 5/4 + j * _width/2,   avFrame->data[2] + j * avFrame->linesize[2], _width/2);
                    }
                }
            }

            _decode_next_frame = false;
            updated = true;
        }

        //_decode_next_frame = _timeline->play();
        _target_pts = 0;

        /*
        bool update = false;
        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }
        */

        return updated;
    }

    void Ffmpeg::execute(VkSemaphore wait_semaphore, VkFence fence) {
        _last_fence = fence;
        submit_compute_buffer(_device->compute_queue(), _compute_command_buffer, wait_semaphore, _compute_complete, fence);
    }

    void Ffmpeg::scrub(const Frame& frame) {
        auto i = frame.index;
        if (i == _current_frame) {
            return;
        } else if (i == _current_frame + 1) {
            _decode_next_frame = true;
            _current_frame = i;
        } else {
            if (i > _frame_count) {
                i = _frame_count - 1;
            }
            int64_t pos = 0;
            if (_video_stream->time_base.num) {
                pos = i * _video_stream->r_frame_rate.den;
            } else {
                pos = i * AV_TIME_BASE;
            }

            //std::cout << "seek pos: " << pos << std::endl;

            if (pos < 0) {
                throw std::runtime_error("uh oh");
            }
            
            if (0 > avformat_seek_file(_format_context, _video_stream->index, INT64_MIN, pos, INT64_MAX, 0)) {
                throw std::runtime_error("failed to seek");
            }

            avcodec_flush_buffers(_codec_context);
            _decode_next_frame = true;
            _current_frame = i;
            
            _target_pts = pos;
            _eof = false;
        }
    }

}