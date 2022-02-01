#include <iostream>

#include "ocio.hpp"
#include "image.hpp"
#include "compute/kernel.hpp"

#include "inputs/image_uploader.hpp"

#include "OpenColorIO/OpenColorIO.h"
namespace OCIO = OCIO_NAMESPACE;

namespace vkd {    

    REGISTER_NODE("ocio", "ocio", Ocio);

    class OcioStatic {
    public:
        OcioStatic() = default;
        ~OcioStatic() = default;
        OcioStatic(OcioStatic&&) = delete;
        OcioStatic(const OcioStatic&) = delete;

        void load() {
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

        size_t num_spaces() const { return _space_names.size(); }
        const auto& space_names() const { return _space_names; }
        const std::string& space_name_at_index(size_t i) const { return _space_names[i]; }

        OCIO::ConstConfigRcPtr get() const { return _config; }
    private:
        OCIO::ConstConfigRcPtr _config = nullptr;
        std::vector<std::string> _space_names;
    };

    const OcioStatic& Ocio::GetOCIOConfig() {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_ocio == nullptr) {
            s_ocio = std::make_unique<OcioStatic>();
            s_ocio->load();
        }
        return *s_ocio;
    }

    std::unique_ptr<OcioStatic> Ocio::s_ocio = nullptr;
    std::mutex Ocio::s_mutex;
    Ocio::Ocio() = default;
    Ocio::~Ocio() = default;

    void Ocio::_make_shader(const std::shared_ptr<Image>& inp, const std::shared_ptr<Image>& outp, int src_index, int dst_index) {
        _ocio_images.clear();

        const char * src_name = GetOCIOConfig().space_name_at_index(src_index).c_str();
        const char * dst_name = GetOCIOConfig().space_name_at_index(dst_index).c_str();

        OCIO::ConstConfigRcPtr config = GetOCIOConfig().get();
    
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

        auto shader = std::make_unique<ManualComputeShader>(_device);
        //std::cout << kernel_text << std::endl;
        shader->create(kernel_text, "main");

        _kernel = Kernel::make(*this, std::move(shader), "OCIO_SHADER", {16, 16, 1});


        int num_tex_2d = desc->getNumTextures();
        int num_tex_3d = desc->getNum3DTextures();

        std::cout << "ocio: num 2d tex - " << num_tex_2d << std::endl;
        std::cout << "ocio: num 3d tex - " << num_tex_3d << std::endl;
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

            auto image = std::make_shared<Image>(_device);
            image->create_image(VK_FORMAT_R32_SFLOAT, {width, height}, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
            image->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            image->create_view(VK_IMAGE_ASPECT_COLOR_BIT);

            auto staging = std::make_shared<vkd::StagingImage>(_device);
            staging->create_image(VK_FORMAT_R32_SFLOAT, {width, height});

            auto ptr = staging->map();
            memcpy(ptr, values, width * height * sizeof(float));
            staging->unmap();

            auto buf = vkd::begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());

            image->copy(*staging, buf);

            image->set_layout(buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            vkd::flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);

            int bind = _kernel->arg_index_for_name(samplerName);
            _ocio_images[bind] = image;

            std::cout << i << ": " << textureName << " " << samplerName << " " << width << " " << height << " " << channel << " " << interpolation << std::endl;
        }

        for (int i = 0; i < num_tex_3d; ++i) {
            // TODO: support 3d luts
        }

        _kernel->set_arg(0, inp);
        _kernel->set_arg(1, outp);

        for (auto&& im : _ocio_images) {
            _kernel->set_arg(im.first, im.second);
        }
    }

    void Ocio::init() {
        _compute_command_buffer = create_command_buffer(_device->logical_device(), _device->command_pool());

        _size = {0, 0};
        
        _compute_complete = Semaphore::make(_device);
        
        auto image = _image_node->get_output_image();
        _size = image->dim();

        _image = vkd::Image::float_image(_device, _size);

        _src_param = make_param<int>(*this, "src space", 0, {"enum"});
        _src_param->as<int>().max(GetOCIOConfig().num_spaces());
        _src_param->as<int>().set_default(0);
        _src_param->enum_names(GetOCIOConfig().space_names());
        _src_param->order(0);
        _dst_param = make_param<int>(*this, "dst space", 0, {"enum"});
        _dst_param->as<int>().max(GetOCIOConfig().num_spaces());
        _dst_param->as<int>().set_default(0);
        _dst_param->enum_names(GetOCIOConfig().space_names());
        _dst_param->order(1);

        _make_shader(image, _image, _src_param->as<int>().get(), _dst_param->as<int>().get());
    }

    void Ocio::allocate(VkCommandBuffer buf) {
        _image->allocate(buf);
    }

    void Ocio::deallocate() { 
        _image->deallocate();
    }

    bool Ocio::update(ExecutionType type) {
        bool update = false;

        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }

        if (update) {
            auto image = _image_node->get_output_image();
            _make_shader(image, _image, _src_param->as<int>().get(), _dst_param->as<int>().get());
        }

        return update;
    }

    void Ocio::execute(ExecutionType type, const SemaphorePtr& wait_semaphore, Fence * fence) {
        begin_command_buffer(_compute_command_buffer);
        _kernel->dispatch(_compute_command_buffer, _size.x, _size.y);
        end_command_buffer(_compute_command_buffer);

        submit_compute_buffer(*_device, _compute_command_buffer, wait_semaphore, _compute_complete, fence);
    }

}