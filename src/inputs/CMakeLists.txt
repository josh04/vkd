set(LIBVKD_inputs_SOURCE
    blockedit.cpp
    exr.cpp
    ffmpeg.cpp
    image_uploader.cpp
    raw.cpp
)

target_sources(vkd PRIVATE ${LIBVKD_inputs_SOURCE})