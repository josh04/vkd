set(LIBVKD_inputs_SOURCE
    blockedit.cpp
    exr.cpp
    ffmpeg.cpp
    image_uploader.cpp
    raw.cpp
)

set(LIBVKD_sane_SOURCE
    sane.cpp
)

target_sources(vkd PRIVATE ${LIBVKD_inputs_SOURCE})

target_sources(vkd PRIVATE ${LIBVKD_sane_SOURCE})
add_subdirectory(sane ${CMAKE_CURRENT_BINARY_DIR}/sane)