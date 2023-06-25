#include <mutex>
#include "ffmpeg.hpp"
#include "image.hpp"
#include "command_buffer.hpp"
#include "buffer.hpp"
#include "compute/kernel.hpp"
#include "imgui/imgui.h"

#include "ocio/ocio_functional.hpp"

extern "C" {
#include <libavformat/avformat.h>
}

#include "ffmpeg_init.hpp"

namespace vkd {
    REGISTER_NODE("ffmpeg", "ffmpeg", Ffmpeg);

    Ffmpeg::Ffmpeg() : _block() {

    }

    void Ffmpeg::post_setup() {
        _block = BlockEditParams{_params, param_hash_name()};
        _path_param = make_param<ParameterType::p_string>(param_hash_name(), "path", 0);
        _path_param->as<std::string>().set_default("test.mp4");
        _path_param->tag("filepath");

        _frame_param = make_param<ParameterType::p_frame>(param_hash_name(), "frame", 0);
        
        _params["_"].emplace(_path_param->name(), _path_param);
        _params["_"].emplace(_frame_param->name(), _frame_param);
    }

    Ffmpeg::~Ffmpeg() {
        avcodec_close(_codec_context);
        av_free(_codec_context);
        avformat_free_context(_format_context);
    }

    void Ffmpeg::init() {
        ffmpeg_static_init();

        if (_path_param->as<std::string>().get().size() < 3) {
            throw GraphException("No path provided to ffmpeg node.");
        } 

        

        _format_context = avformat_alloc_context();
        
        if (avformat_open_input(&_format_context, _path_param->as<std::string>().get().c_str(), nullptr, nullptr) != 0) {
            throw GraphException("Error while calling avformat_open_input (probably invalid file format)");
        }
        
        if (avformat_find_stream_info(_format_context, nullptr) < 0) {
            throw GraphException("Error while calling avformat_find_stream_info");
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
            throw GraphException("Didn't find video streams in the file (probably audio file)");
        }
        
        // getting the required codec structure
        const auto codecL = avcodec_find_decoder(_video_stream->codecpar->codec_id);
        if (codecL == nullptr) {
            throw GraphException("Codec required by video file not available");
        }
        
        // allocating a structure
        _codec_context = avcodec_alloc_context3(codecL);
        
        _codec_context->extradata = _video_stream->codecpar->extradata;
        _codec_context->extradata_size = _video_stream->codecpar->extradata_size;
        
        // initializing the structure by opening the codec
        if (avcodec_open2(_codec_context, codecL, nullptr) < 0) {
            throw GraphException("Could not open codec");
        }
        
        _width = _video_stream->codecpar->width;
        _height = _video_stream->codecpar->height;
        
        _frame_count = _video_stream->nb_frames + 1 - ((_video_stream->nb_frames + 1) % 2);

        if (_frame_count == 0) {
            _frame_count = (_format_context->duration * _video_stream->avg_frame_rate.num / _video_stream->avg_frame_rate.den) / 1000000;
        }

        _block.total_frame_count->as<int>().set_force(_frame_count > 0 ? _frame_count : 100);
        

        _uploader = std::make_unique<ImageUploader>(_device);
        _uploader->init(_width, _height, ImageUploader::InFormat::yuv420p, ImageUploader::OutFormat::float32, param_hash_name());
        for (auto&& kern : _uploader->kernels()) {
            register_params(*kern);
        }

        //_local_timeline->frame_count = _frame_count;

        //_local_timeline->name = _path_param->as<std::string>().get().c_str();

        _current_frame = 0;

        _ocio = std::make_unique<OcioNode>(OcioNode::Type::In);
        _ocio->init(*this);
    }

    bool Ffmpeg::update(ExecutionType type) {
        bool updated = false;

        auto frame = _frame_param->as<Frame>().get();
        scrub(_block.translate(frame, _frame_count > 0));

        auto start_block = _block.frame_start_block->as<Frame>().get();
        auto end_block = _block.frame_end_block->as<Frame>().get();
        if (frame.index < start_block.index || frame.index > end_block.index) {
            memset(_uploader->get_main(), 0, _buffer_size);
            if (!_blanked) {
                _blanked = true;
                updated = true;
            }
            return updated;
        }
        _blanked = false;

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
                    if (_eof) { // we're at the end of the stream
                        int retc = avcodec_send_packet(_codec_context, nullptr);
                        if (retc == AVERROR(EAGAIN)) {
                            continue;
                        } else if (retc == AVERROR_EOF) {
                            continue;
                        } else if (retc < 0) {
                            throw UpdateException("send packet failed");
                        }
                    } else { // else get a packet
                        if (av_read_frame(_format_context, packet.get()) < 0) {
                            _eof = true;
                            continue;
                            //throw GraphException("ran out of packets");
                        }
                    }
                    if (packet->stream_index == _video_stream->index) {
                        int retc = avcodec_send_packet(_codec_context, packet.get());
                        if (retc == AVERROR(EAGAIN)) {
                            continue;
                        } else if (retc == AVERROR_EOF) {
                            return false;
                        } else if (retc < 0) {
                            throw UpdateException("send packet failed");
                        }
                    }
                } else if (ret == AVERROR_EOF) {
                    return false;
                } else if (ret == AVERROR(EINVAL)) {
                    throw GraphException("receive frame failed");
                } else if (avFrame->pkt_pts < _target_pts) {
                    //console << "target pts: " << _target_pts << "pts: " << packet->pts << " dts: " << packet->pts << std::endl;
                    //console << "skippe frame: " << avFrame->pkt_pts << std::endl;
                    continue;
                } else if (ret == 0) {
                    got_frame = true;
                }
            }

            //console << "pts: " << packet->pts << " dts: " << packet->pts << std::endl;
            //console << "coded frame number: " << avFrame->coded_picture_number << std::endl;

            // do something with frame

            //console << "got frame:" << _video_stream->codecpar->format << std::endl;
            //console << "avframe:" << _codec_context->pix_fmt << std::endl;

            if (got_frame) {
                for (int j = 0; j < _height; ++j) {
                    memcpy((uint8_t*)_uploader->get_main() + j * _width, avFrame->data[0] + j * avFrame->linesize[0], _width);
                    if (j < _height / 2) {
                        memcpy((uint8_t*)_uploader->get_main() + _width * _height       + j * _width/2,   avFrame->data[1] + j * avFrame->linesize[1], _width/2);
                        memcpy((uint8_t*)_uploader->get_main() + _width * _height * 5/4 + j * _width/2,   avFrame->data[2] + j * avFrame->linesize[2], _width/2);
                    }
                }
            }

            _decode_next_frame = false;
            updated = true;
        }

        //_decode_next_frame = _timeline->play();
        _target_pts = 0;

        
        bool update = false;
        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }

        if (update) {
            _ocio->update(*this, _uploader->get_gpu());
        }
        

        return updated || update;
    }

    void Ffmpeg::allocate(VkCommandBuffer buf) {
        _uploader->allocate(buf);
    }

    void Ffmpeg::deallocate() { 
        _uploader->deallocate();
    }

    void Ffmpeg::execute(ExecutionType type, Stream& stream) {
        command_buffer().begin();
        _uploader->commands(command_buffer());
        _ocio->execute(command_buffer(), _width, _height);
        command_buffer().end();

        stream.submit(command_buffer());
    }

    void Ffmpeg::scrub(const Frame& frame) {
        auto i = frame.index;

        if (_frame_count > 0 && i > _block.total_frame_count->as<int>().get()) {
            i = _block.total_frame_count->as<int>().get() - 1;
        }

        i = std::max((int64_t)0, i);

        if (i == _current_frame) {
            return;
        } else if (i == _current_frame + 1) {
            _decode_next_frame = true;
            _current_frame = i;
        } else {
            
            int64_t pos = 0;
            if (_video_stream->avg_frame_rate.num) {
                //pos = av_rescale_q(i * _video_stream->time_base.den / _video_stream->time_base.num, AVRational{1, AV_TIME_BASE}, _video_stream->time_base);
                pos = i * (_video_stream->time_base.den * _video_stream->avg_frame_rate.den) / (_video_stream->time_base.num * _video_stream->avg_frame_rate.num);
            } else {
                pos = i * AV_TIME_BASE;
            }

            //console << "seek pos: " << pos << std::endl;

            if (pos < 0) {
                throw GraphException("uh oh");
            }
            
            if (0 > avformat_seek_file(_format_context, _video_stream->index, INT64_MIN, pos, INT64_MAX, 0)) {
                throw GraphException("failed to seek");
            }

            avcodec_flush_buffers(_codec_context);
            _decode_next_frame = true;
            _current_frame = i;
            
            _target_pts = pos;
            _eof = false;
        }
    }

}