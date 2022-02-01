set(LIBVKD_outputs_SOURCE
    exr.cpp
    ffmpeg.cpp
    image_downloader.cpp
    immediate_exr.cpp
)

target_sources(vkd PRIVATE ${LIBVKD_outputs_SOURCE})