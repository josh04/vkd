#include <mutex>
#include "ffmpeg_loop.hpp"
#include "image.hpp"
#include "command_buffer.hpp"
#include "buffer.hpp"
#include "compute/kernel.hpp"
#include "imgui/imgui.h"

#include "ocio/ocio_functional.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}

#include "ffmpeg_init.hpp"

#include <random>

namespace vkd {
    REGISTER_NODE("ffmpeg_loop", "ffmpeg_loop", FfmpegLoop);

    FfmpegLoop::FfmpegLoop() {

    }

    void FfmpegLoop::post_setup() {
    }

    FfmpegLoop::~FfmpegLoop() {
        avcodec_close(_enc_codec_context);
        av_free(_enc_codec_context);
        avcodec_close(_dec_codec_context);
        av_free(_dec_codec_context);
    }

    void FfmpegLoop::init() {
        ffmpeg_static_init();

        auto codec_id = AV_CODEC_ID_H264;
        
        const auto codec_dec = avcodec_find_decoder(codec_id);
        if (codec_dec == nullptr) {
            throw GraphException("Codec required by video file not available");
        }
        
        _dec_codec_context = avcodec_alloc_context3(codec_dec);
        
        if (avcodec_open2(_dec_codec_context, codec_dec, nullptr) < 0) {
            throw GraphException("Could not open codec");
        }

        auto image = _image_node->get_output_image();
        _size = image->dim();
        _dl_image = Image::float_image(_device, _size, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        const auto codec_enc = avcodec_find_encoder(codec_id);
        if (codec_enc == nullptr) {
            throw GraphException("Codec required by video file not available");
        }

        _enc_codec_context = avcodec_alloc_context3(codec_enc);
        _enc_codec_context->codec_id = codec_id;
        
        _enc_codec_context->width = _size.x;
        _enc_codec_context->height = _size.y;
        
        //_enc_codec_context->bit_rate = _bitrate;
        
        _enc_codec_context->time_base.den = 25;
        _enc_codec_context->time_base.num = 1;
        
        _enc_codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
        _enc_codec_context->level = 52;
        
        AVDictionary * _dict = NULL;

        av_dict_set(&_dict, "vprofile", "high", 0);
        av_dict_set(&_dict, "preset", "slow", 0);
        av_dict_set(&_dict, "crf", "24", 0);

        av_dict_set(&_dict, "annex_b", "1", 0);
        av_dict_set(&_dict, "repeat_headers", "1", 0);
        av_dict_set(&_dict, "tune", "zerolatency", 0);
        
        if (avcodec_open2(_enc_codec_context, codec_enc, &_dict) < 0) {
            throw GraphException("Could not open codec");
        }

        _downloader = std::make_unique<ImageDownloader>(_device);
        _downloader->init(_dl_image, ImageDownloader::OutFormat::yuv420p, param_hash_name());
        for (auto&& kern : _downloader->kernels()) {
            register_params(*kern);
        }

        _uploader = std::make_unique<ImageUploader>(_device);
        _uploader->init(_size.x, _size.y, ImageUploader::InFormat::yuv420p, ImageUploader::OutFormat::float32, param_hash_name());
        for (auto&& kern : _uploader->kernels()) {
            register_params(*kern);
        }

        _ocio_out = std::make_unique<OcioNode>(OcioNode::Type::Out);
        _ocio_out->init(*this, ocio_functional::default_input_space_index());
        _ocio_in = std::make_unique<OcioNode>(OcioNode::Type::In);
        _ocio_in->init(*this, ocio_functional::default_input_space_index());
        
        _blank_offset = make_param<int>(*this, "blank offset", 0);
        _blank_offset->as<int>().set_default(100);
        _blank_offset->as<int>().soft_min(0);
        _blank_offset->as<int>().soft_max(10000);

        _blank_size = make_param<int>(*this, "blank NAL count", 0);
        _blank_size->as<int>().set_default(50);
        _blank_size->as<int>().soft_min(0);
        _blank_size->as<int>().soft_max(1000);

        _frame_param = make_param<ParameterType::p_frame>(*this, "frame", 0);

        _unloop_param = make_param<ParameterType::p_bool>(*this, "disable ffmpeg", 0);
    }

    bool FfmpegLoop::update(ExecutionType type) {
        
        bool update = false;
        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.first != "frame" && el.second->changed()) {
                    update = true;
                }
            }
        }

        if (update) {
            _ocio_out->update(*this, _image_node->get_output_image(), _dl_image);
            _ocio_in->update(*this, _uploader->get_gpu());
        }
        
        return update;
    }

    void FfmpegLoop::allocate(VkCommandBuffer buf) {
        _downloader->allocate(buf);
        _uploader->allocate(buf);
        _dl_image->allocate(buf);
    }

    void FfmpegLoop::deallocate() {
        _downloader->deallocate();
        _uploader->deallocate();
        _dl_image->deallocate();
    }

    void FfmpegLoop::execute(ExecutionType type, Stream& stream) {
        command_buffer().begin();
        _ocio_out->execute(command_buffer(), _size.x, _size.y);
        _downloader->commands(command_buffer());
        command_buffer().end();

        stream.submit(command_buffer());
        stream.flush();

        std::shared_ptr<AVFrame> avFrame(av_frame_alloc(), [](AVFrame* a){ av_frame_free(&a); });

        avFrame->format = AV_PIX_FMT_YUV420P;
        avFrame->width = _size.x;
        avFrame->height = _size.y;
        static int64_t pts = 0;
        avFrame->pts = pts++;
        
        av_image_fill_arrays(avFrame->data, avFrame->linesize, (const uint8_t *)_downloader->get_main(), AV_PIX_FMT_YUV420P, _size.x, _size.y, 1);

        int ret = avcodec_send_frame(_enc_codec_context, avFrame.get());
        if (ret < 0) {
            console << "failed to send frame in ffmpeg_loop" << std::endl;
        }

        std::vector<AVPacket*> packets;

        int ret2 = 0;
        while (ret2 == 0) {
            AVPacket * pkt = av_packet_alloc();
            ret2 = avcodec_receive_packet(_enc_codec_context, pkt);
            if (ret2 == 0) {
                packets.push_back(pkt);
            } else if (ret2 == AVERROR(EAGAIN) || ret2 == 0) {
                continue;
            } else {
                console << "failed to receive packet in ffmpeg_loop" << std::endl;
            }
        }

        for(auto&& pkt : packets) {

            std::vector<int> pointers;
            for (int i = 0; i < pkt->size; ++i) {
                if (pkt->data[i] == 0x1) {
                    pointers.push_back(i);
                }
            }

            static std::default_random_engine engine(0);//_frame_param->as<Frame>().get().index : (unsigned)time(nullptr));
            //std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            std::uniform_int_distribution<int> remove_packet_count(0, std::max(_blank_size->as<int>().get(), 0));
            std::uniform_int_distribution<int> remove_packet_index(0, pointers.size() - 1);

            int remove_count = remove_packet_count(engine);

            for (int i = 0; i < remove_count; ++i) {
                int index = remove_packet_index(engine);

                int to_remove_from = pointers[index] + 1;

                if (index == pointers.size() - 1) {
                    //console << "pkt size " << pkt->size << "last chunk blanking from " << to_remove_from << " to " << pkt->size - 1 << " diff " << pkt->size - to_remove_from - 1 << std::endl;
                    memset(&pkt->data[to_remove_from], 0, std::max(pkt->size - to_remove_from - 1, 0));
                } else {
                    //console << "pkt size " << pkt->size << " chunk blanking from " << to_remove_from << " to " << pointers[index + 1] - 1 << " diff " << std::min(pointers[index + 1] - to_remove_from - 1, pkt->size - to_remove_from - 1) << std::endl;
                    memset(&pkt->data[to_remove_from], 0, std::max(std::min(pointers[index + 1] - to_remove_from - 1, pkt->size - to_remove_from - 1), 0));
                }
            }

            /* auto offset = _blank_offset->as<int>().get();
            auto size = _blank_size->as<int>().get();

            auto act_offset = std::max(std::min(offset, pkt->size), 0);
            auto act_size = std::min(std::max(pkt->size - act_offset, 0), size);

            for (int i = 0; i < act_size; ++i) {
                if (pkt->data[act_offset + i] == 0x1) {
                    break;
                }
                pkt->data[act_offset + i] = 0;
            } */
            
            int ret3 = avcodec_send_packet(_dec_codec_context, pkt);
            if (ret3 == AVERROR(EAGAIN)) {
                // uhhhh
            } else if (ret3 < 0) {
                console << "failed to send packet in ffmpeg_loop" << std::endl;
            }

            av_packet_unref(pkt);
        }

        memset (_uploader->get_main(), 0, _uploader->get_main_size());

        AVFrame * frame = av_frame_alloc();
        int ret4 = avcodec_receive_frame(_dec_codec_context, frame);
        AVFrame * frameToUse = frame;

        if (_unloop_param->as<bool>().get()) {
            frameToUse = avFrame.get();
        }

        if (ret4 == 0) {
            for (int j = 0; j < _size.y; ++j) {
                memcpy((uint8_t*)_uploader->get_main() + j * _size.x, frameToUse->data[0] + j * frameToUse->linesize[0], _size.x);
                if (j < _size.y / 2) {
                    memcpy((uint8_t*)_uploader->get_main() + _size.x * _size.y       + j * _size.x/2,   frameToUse->data[1] + j * frameToUse->linesize[1], _size.x/2);
                    memcpy((uint8_t*)_uploader->get_main() + _size.x * _size.y * 5/4 + j * _size.x/2,   frameToUse->data[2] + j * frameToUse->linesize[2], _size.x/2);
                }
            } 
        } else {
            console << "failed to receive frame in ffmpeg_loop" << std::endl;
            return;
        }

        command_buffer().begin();
        _uploader->commands(command_buffer());
        _ocio_in->execute(command_buffer(), _size.x, _size.y);
        command_buffer().end();

        stream.submit(command_buffer());

        av_frame_unref(frame);


        /* if (got_frame) {
            for (int j = 0; j < _height; ++j) {
                memcpy((uint8_t*)_uploader->get_main() + j * _width, avFrame->data[0] + j * avFrame->linesize[0], _width);
                if (j < _height / 2) {
                    memcpy((uint8_t*)_uploader->get_main() + _width * _height       + j * _width/2,   avFrame->data[1] + j * avFrame->linesize[1], _width/2);
                    memcpy((uint8_t*)_uploader->get_main() + _width * _height * 5/4 + j * _width/2,   avFrame->data[2] + j * avFrame->linesize[2], _width/2);
                }
            }
        } */
    }

}