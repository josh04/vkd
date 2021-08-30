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
    REGISTER_NODE("ffmpeg_output", "ffmpeg_output", FfmpegOutput);

    FfmpegOutput::FfmpegOutput() {
        _path_param = make_param<ParameterType::p_string>(param_hash_name(), "path", 0);
        _path_param->as<std::string>().set_default("test1111.mp4");
        _path_param->tag("filepath");

        _params["_"].emplace(_path_param->name(), _path_param);
    }

    FfmpegOutput::~FfmpegOutput() {
        if (_codec_context) {
            avcodec_close(_codec_context);
            //avcodec_free_context(&_codec_context);
        }
        if (_format_context) {
            avformat_free_context(_format_context);
        }
    }

    void FfmpegOutput::init() {
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

        auto sz = _buffer_node_sz->get_image_size();
        _width = sz[0];
        _height = sz[1];


        // avcodec

        AVCodecID codec_id = AV_CODEC_ID_H264;

        _codec = avcodec_find_encoder(codec_id);

        _codec_context = avcodec_alloc_context3(_codec);
        
        _codec_context->codec_id = codec_id;
        
        _codec_context->width = _width;
        _codec_context->height = _height;
        
        //_codec_context->bit_rate = _bitrate;
        
        _codec_context->time_base.den = _fps;
        _codec_context->time_base.num = 1;
        
        _codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
        _codec_context->level = 52;

        AVDictionary * _dict = NULL;

        av_dict_set(&_dict, "vprofile", "high", 0);
        av_dict_set(&_dict, "preset", "slow", 0);    
        av_dict_set(&_dict, "crf", "24", 0);
        
        int err = avcodec_open2(_codec_context, _codec, &_dict);
        if (err < 0) {
            std::cerr << "avcodec_open2 failed: " << err << std::endl;
            throw 0;
        }

        // avformat

        avformat_alloc_output_context2(&_format_context, NULL, "mp4", NULL);
        _video_stream = avformat_new_stream(_format_context, _codec_context->codec);
        _video_stream->codec = _codec_context;
        _video_stream->id = 0;

        std::string path = _path_param->as<std::string>().get();

        if (path.size() < 3) {
            throw GraphException("No path provided to ffmpeg node.");
        } 

        if (avio_open(&_format_context->pb, path.c_str(), AVIO_FLAG_WRITE) < 0) {
            throw GraphException("FFMPEG: File output open failed.");
        }

        int err2 = avformat_write_header(_format_context, NULL);
        if (err2 < 0) {
            throw GraphException("FFMPEG: Failed to write header." + std::to_string(err2));
        }

        _fence = Fence::create(_device, false);
    }

    void FfmpegOutput::post_init()
    {
    }

    bool FfmpegOutput::update(ExecutionType type) {
        bool updated = false;

        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    updated = true;
                }
            }
        }


        return updated;
    }

    void FfmpegOutput::execute(ExecutionType type, VkSemaphore wait_semaphore, Fence * fence) {
        if (type != ExecutionType::Execution) {
            submit_compute_buffer(_device->compute_queue(), VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, fence);
            return;
        }

        submit_compute_buffer(_device->compute_queue(), VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, _fence.get());
        
        if (_fence) {
            _fence->wait();
            _fence->reset();        
        }

        uint8_t * buffer = (uint8_t *)_buffer_node->get_output_buffer()->get();

        std::shared_ptr<AVFrame> avFrame(av_frame_alloc(), [](AVFrame* a){ av_frame_free(&a); });
        
        avFrame->data[0] = buffer;
        //memset(buffer, 255, _width * _height);
        avFrame->data[1] = buffer + _width * _height;
        //memset(avFrame->data[1], 127, _width * _height * 1 / 4);
        avFrame->data[2] = buffer + _width * _height * 5 / 4;
        //memset(avFrame->data[2], 127, _width * _height * 1 / 4);

        avFrame->linesize[0] = _width;
        avFrame->linesize[1] = (_width / 2);
        avFrame->linesize[2] = (_width / 2);
        
        avFrame->format = _codec_context->pix_fmt;
        avFrame->width = _codec_context->width;
        avFrame->height = _codec_context->height;

        avFrame->pts = _frame_count;
        _frame_count++;

		AVFrame * frame_pointer = avFrame.get();

        int send_err = avcodec_send_frame(_codec_context, frame_pointer);
		if (send_err != 0) {
			std::cerr << "avcodec_send_frame error" << std::endl;
		}
        
        int packet_err = 0;
        
        AVPacket * packet = av_packet_alloc();
        while (packet_err == 0) {
            packet_err = avcodec_receive_packet(_codec_context, packet);
            // handle proper errors here
            // write packets here
            if (packet_err == 0)
            {
                std::cout << "got a packet, size " << packet->size << std::endl;
                AVRational time_base = _codec_context->time_base;
                        
                packet->pts = av_rescale_q_rnd(packet->pts, time_base, _video_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                packet->dts = av_rescale_q_rnd(packet->dts, time_base, _video_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

                packet->duration = av_rescale_q(packet->duration, time_base, _video_stream->time_base);
                packet->stream_index = _video_stream->index;
                
                av_interleaved_write_frame(_format_context, packet);
            }
        }

        submit_compute_buffer(_device->compute_queue(), VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, fence);
    }

    void FfmpegOutput::finish() {

        int send_err = avcodec_send_frame(_codec_context, nullptr);
		if (send_err != 0) {
			std::cerr << "avcodec_send_frame error" << std::endl;
		}
        
        int packet_err = 0;
        AVPacket * packet = av_packet_alloc();
        while (packet_err == 0) {
            packet_err = avcodec_receive_packet(_codec_context, packet);
            // handle proper errors here
            // write packets here
            if (packet_err == 0)
            {
                std::cout << "got a packet" << std::endl;
                AVRational time_base = _codec_context->time_base;
                        
                packet->pts = av_rescale_q_rnd(packet->pts, time_base, _video_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                packet->dts = av_rescale_q_rnd(packet->dts, time_base, _video_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

                packet->duration = av_rescale_q(packet->duration, time_base, _video_stream->time_base);
                packet->stream_index = _video_stream->index;
                
                av_interleaved_write_frame(_format_context, packet);
            }
        }

        av_write_trailer(_format_context);
		avio_close(_format_context->pb);
    }

}