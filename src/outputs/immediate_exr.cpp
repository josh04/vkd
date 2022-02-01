#include "immediate_exr.hpp"
#include "image.hpp"
#include "image_downloader.hpp"
#include "command_buffer.hpp"

#include "ImfRgbaFile.h"

#include "png.h"
#include "TaskScheduler.h"

#include "ghc/filesystem.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace vkd {
    std::string immediate_exr(const std::shared_ptr<Device>& device, std::string filename, ImmediateFormat format, const std::shared_ptr<Image>& image, std::unique_ptr<enki::TaskSet>& task) {
        auto downloader = std::make_shared<ImageDownloader>(device);
        std::string ext;
        if (format == ImmediateFormat::EXR) {
            downloader->init(image, ImageDownloader::OutFormat::half_rgba, "immediate_exr");
            ext = "exr";
        } else if (format == ImmediateFormat::PNG) {
            downloader->init(image, ImageDownloader::OutFormat::uint8_rgbx, "immediate_png");
            ext = "png";
        } else if (format == ImmediateFormat::JPG) {
            downloader->init(image, ImageDownloader::OutFormat::uint8_rgbx, "immediate_jpg");
            ext = "jpg";
        }
        downloader->execute();

        Fence::auto_wait(device);

        auto sz = image->dim();

        ghc::filesystem::path path{filename};
        path.replace_extension(ext);
        auto test_path = path;
        int i = 1;
        while (ghc::filesystem::exists(test_path)) {
            test_path = path;
            test_path.replace_filename(test_path.stem().string() + std::string("_") + std::to_string(i));
            test_path.replace_extension(ext);
            i++;
        }
        
        task = std::make_unique<enki::TaskSet>(1, [test_path, downloader, sz, format](enki::TaskSetPartition range, uint32_t threadnum) mutable {

            uint8_t * buffer = (uint8_t *)downloader->get_main();
            try {
                if (format == ImmediateFormat::EXR) {
                    Imf::RgbaOutputFile file(test_path.string().c_str(), sz[0], sz[1], Imf::WRITE_RGBA, 1.0f, Imath::V2f (0, 0), 1.0f, Imf::INCREASING_Y, Imf::ZIP_COMPRESSION);
                    file.setFrameBuffer((Imf::Rgba* )buffer, 1, sz[0]);
                    file.writePixels(sz[1]);
                } else if (format == ImmediateFormat::PNG) {

                    FILE * fp = fopen(test_path.string().c_str(), "wb");
                    if (fp == NULL) {
                        throw std::runtime_error("PNG: Could not open file for writing.");
                    }

                    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
                    if (png_ptr == NULL) {
                        throw std::runtime_error("PNG: Could not allocate write struct.");
                    }

                    png_infop info_ptr = png_create_info_struct(png_ptr);
                    if (info_ptr == NULL) {
                        throw std::runtime_error("PNG: Could not allocate info struct.");
                    }
                    png_init_io(png_ptr, fp);

                    // Write header (8 bit colour depth)
                    png_set_IHDR(png_ptr, info_ptr, sz[0], sz[1], 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

                    png_write_info(png_ptr, info_ptr);

                    for (int y = 0; y < sz[1]; y++) {
                        png_write_row(png_ptr, buffer + y * sz[0] * 4);
                    }

                    png_write_end(png_ptr, NULL);

                    fclose(fp);
                } else if (format == ImmediateFormat::JPG) {          
                    stbi_write_jpg(test_path.string().c_str(), sz[0], sz[1], 4, buffer, 80);
                }
            } catch (...) {
                
            }
        });

        ts().AddTaskSetToPipe(task.get());
        
        return test_path.string();
    }
}