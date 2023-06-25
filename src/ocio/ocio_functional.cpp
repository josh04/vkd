#include <string>
#include <iostream>

#include "image.hpp"
#include "device.hpp"

#include "ocio_functional.hpp"
#include "ocio_static.hpp"
#include "shader.hpp"
#include "compute/kernel.hpp"
#include "make_param.hpp"

namespace vkd {
    namespace ocio_functional {
        namespace {
            std::string _working_space = "";
            int32_t _working_space_index = -1;
            std::string _default_input_space = "";
            int32_t _default_input_space_index = -1;
            std::string _display_space = "";
            int32_t _display_space_index = -1;
            std::string _scan_space = "";
            int32_t _scan_space_index = -1;
            std::string _screenshot_space = "";
            int32_t _screenshot_space_index = -1;
        }

        std::shared_ptr<ParameterInterface> make_ocio_param(EngineNode& node, const std::string& name) {
            return make_ocio_param(node, name, 0);
        }

        std::shared_ptr<ParameterInterface> make_ocio_param(EngineNode& node, const std::string& name, int default_index) {
            auto ocio_param = make_param<int>(node, name, 0, {"enum"});
            ocio_param->as<int>().max(OcioStatic::GetOCIOConfig().num_spaces());
            ocio_param->as<int>().set_default(default_index);
            ocio_param->enum_names(OcioStatic::GetOCIOConfig().space_names());
            return ocio_param;
        }

        const std::string& working_space() { return _working_space; }
        int32_t working_space_index() { return _working_space_index; }
        const std::string& default_input_space() { return _default_input_space; }
        int32_t default_input_space_index() { return _default_input_space_index; }
        const std::string& display_space() { return _display_space; }
        int32_t display_space_index() { return _display_space_index; }
        const std::string& scan_space() { return _scan_space; }
        int32_t scan_space_index() { return _scan_space_index; }
        const std::string& screenshot_space() { return _screenshot_space; }
        int32_t screenshot_space_index() { return _screenshot_space_index; }

        bool set_working_space(const std::string& sp) { 
            int32_t indx = OcioStatic::GetOCIOConfig().index_from_space_name(sp);
            if (indx == -1) { 
                _working_space = OcioStatic::GetOCIOConfig().space_name_at_index(0);
                _working_space_index = 0;
                return false; 
            }
            _working_space = sp; 
            _working_space_index = indx;
            return true;
        }

        bool set_default_input_space(const std::string& sp) { 
            int32_t indx = OcioStatic::GetOCIOConfig().index_from_space_name(sp);
            if (indx == -1) { 
                _default_input_space = OcioStatic::GetOCIOConfig().space_name_at_index(0);
                _default_input_space_index = 0;
                return false; 
            }
            _default_input_space = sp; 
            _default_input_space_index = indx;
            return true;
        }

        bool set_display_space(const std::string& sp) { 
            int32_t indx = OcioStatic::GetOCIOConfig().index_from_space_name(sp);
            if (indx == -1) { 
                _display_space = OcioStatic::GetOCIOConfig().space_name_at_index(0);
                _display_space_index = 0;
                return false; 
            }
            _display_space = sp; 
            _display_space_index = indx;
            return true;
        }

        bool set_scan_space(const std::string& sp) { 
            int32_t indx = OcioStatic::GetOCIOConfig().index_from_space_name(sp);
            if (indx == -1) { 
                _scan_space = OcioStatic::GetOCIOConfig().space_name_at_index(0);
                _scan_space_index = 0;
                return false; 
            }
            _scan_space = sp; 
            _scan_space_index = indx;
            return true;
        }

        bool set_screenshot_space(const std::string& sp) { 
            int32_t indx = OcioStatic::GetOCIOConfig().index_from_space_name(sp);
            if (indx == -1) { 
                _screenshot_space = OcioStatic::GetOCIOConfig().space_name_at_index(0);
                _screenshot_space_index = 0;
                return false; 
            }
            _screenshot_space = sp; 
            _screenshot_space_index = indx;
            return true;
        }

        std::shared_ptr<Kernel> make_shader(EngineNode& node, const std::shared_ptr<Image>& inp, const std::shared_ptr<Image>& outp, int src_index, int dst_index, std::map<int, std::shared_ptr<Image>>& ocio_images) {
            auto device = inp->device();
            std::shared_ptr<Kernel> kernel = nullptr;

            const char * src_name = OcioStatic::GetOCIOConfig().space_name_at_index(src_index).c_str();
            const char * dst_name = OcioStatic::GetOCIOConfig().space_name_at_index(dst_index).c_str();

            OCIO::ConstConfigRcPtr config = OcioStatic::GetOCIOConfig().get();
        
            // Get the processor corresponding to this transform.
            OCIO::ConstProcessorRcPtr processor = config->getProcessor(src_name, dst_name);
            //OCIO::ConstProcessorRcPtr processor = config->getProcessor(OCIO::ROLE_COMPOSITING_LOG, OCIO::ROLE_SCENE_LINEAR);

            OCIO::GpuShaderDescRcPtr desc = OCIO::GpuShaderDesc::CreateShaderDesc();
            desc->setLanguage(OCIO::GpuLanguage::GPU_LANGUAGE_GLSL_4_0);
            // Get the corresponding CPU processor for 32-bit float image processing.
            OCIO::ConstGPUProcessorRcPtr gpuProcessor = processor->getDefaultGPUProcessor();
            gpuProcessor->extractGpuShaderInfo(desc);

            std::string ocio_kernel = desc->getShaderText();

            std::string kernel_prefix = R"src(#version 450 
                
    layout(rgba32f) uniform image2D inputTex;
    layout(rgba32f) uniform image2D outputTex;

    layout (local_size_x_id = 0, local_size_y_id = 1, local_size_z_id = 2) in;

    layout (push_constant) uniform PushConstants {
        ivec4 vkd_offset;
    } push;

    )src";
            std::string kernel_postfix = R"src(

    void main() 
    {
        ivec2 coord = ivec2(gl_GlobalInvocationID.xy) + push.vkd_offset.xy;
        
        vec4 inp = imageLoad(inputTex, coord);
        vec4 inp2 = OCIOMain(inp);

        imageStore(outputTex, coord, inp2);
    }
            
            )src";

            std::string kernel_text = kernel_prefix + ocio_kernel + kernel_postfix;

            auto shader = std::make_unique<ManualComputeShader>(device);
            //console << kernel_text << std::endl;
            shader->create(kernel_text, "main");

            kernel = Kernel::make(node, std::move(shader), "OCIO_SHADER", Kernel::default_local_sizes);


            int num_tex_2d = desc->getNumTextures();
            int num_tex_3d = desc->getNum3DTextures();

            //console << "ocio: num 2d tex - " << num_tex_2d << std::endl;
            //console << "ocio: num 3d tex - " << num_tex_3d << std::endl;
            for (int i = 0; i < num_tex_2d; ++i) {
                /*

        virtual void getTexture(unsigned index,
                                const char *& textureName,
                                const char *& samplerName,
                                unsigned & width,
                                unsigned & height,
                                TextureType & channel,
                                Interpolation & interpolation) const = 0;
        virtual void getTextureValues(unsigned index, const float *& values) const = 0;
        */
                const char * textureName;
                const char * samplerName;
                uint32_t width;
                uint32_t height;
                OCIO::GpuShaderCreator::TextureType channel;
                OCIO::Interpolation interpolation;
                desc->getTexture(i, textureName, samplerName, width, height, channel, interpolation);
                const float * values = nullptr;
                desc->getTextureValues(i, values);

                auto image = std::make_shared<Image>(device);
                image->create_image(VK_FORMAT_R32_SFLOAT, {width, height}, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
                image->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                image->create_view(VK_IMAGE_ASPECT_COLOR_BIT);

                auto staging = std::make_shared<vkd::StagingImage>(device);
                staging->create_image(VK_FORMAT_R32_SFLOAT, {width, height});

                auto ptr = staging->map();
                memcpy(ptr, values, width * height * sizeof(float));
                staging->unmap();

                auto buf = vkd::begin_immediate_command_buffer(device->logical_device(), device->command_pool());

                image->copy(*staging, buf);

                image->set_layout(buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

                vkd::flush_command_buffer(device->logical_device(), device->queue(), device->command_pool(), buf);

                int bind = kernel->arg_index_for_name(samplerName);
                ocio_images[bind] = image;

                //console << i << ": " << textureName << " " << samplerName << " " << width << " " << height << " " << channel << " " << interpolation << std::endl;
            }

            for (int i = 0; i < num_tex_3d; ++i) {
                // TODO: support 3d luts
            }

            kernel->set_arg(0, inp);
            kernel->set_arg(1, outp);

            for (auto&& im : ocio_images) {
                kernel->set_arg(im.first, im.second);
            }

            return kernel;
        }

    }

    void OcioNode::init(EngineNode& node) {
        if (_type == Type::In) {
            init(node, ocio_functional::default_input_space_index());
        } else {
            init(node, ocio_functional::display_space_index());
        }
    }

    void OcioNode::init(EngineNode& node, int32_t index) {
        _ocio_params = std::make_unique<OcioParams>(node);

        _use_ocio_param = make_param<bool>(node, "use ocio", 0, {});
        _use_ocio_param->as<bool>().set_default(true);

        if (_type == Type::In) {
            _ocio_in_space = ocio_functional::make_ocio_param(node, "input colour space", index);
        } else {
            _ocio_in_space = ocio_functional::make_ocio_param(node, "output colour space", index);
        }
    }

    void OcioNode::update(EngineNode& node, std::shared_ptr<vkd::Image> image) {
        update(node, image, image);
    }

    void OcioNode::update(EngineNode& node, std::shared_ptr<vkd::Image> image_in, std::shared_ptr<vkd::Image> image_out) {
        _ocio_images.clear();

        if (_use_ocio_param->as<bool>().get()) {
            try {
                if (_type == Type::In) {
                    _ocio_in_transform = ocio_functional::make_shader(node, image_in, image_out, _ocio_in_space->as<int>().get(), _ocio_params->working_space_index(), _ocio_images);
                } else {
                    _ocio_in_transform = ocio_functional::make_shader(node, image_in, image_out, _ocio_params->working_space_index(), _ocio_in_space->as<int>().get(), _ocio_images);
                }
            } catch (OcioException& e) {
                std::stringstream strm;
                strm << "OCIO Shader build failed in vkd: " << e.what();
                throw GraphException(strm.str());
            } catch (OCIO::Exception& e) {
                std::stringstream strm;
                strm << "OCIO Shader build failed in OCIO: " << e.what();
                throw GraphException(strm.str());
            }
        }
    }

    void OcioNode::execute(CommandBuffer& buf, int32_t width, int32_t height) {
        if (_use_ocio_param->as<bool>().get() && _ocio_in_transform) {
            _ocio_in_transform->dispatch(buf, width, height);
        }
    }
}