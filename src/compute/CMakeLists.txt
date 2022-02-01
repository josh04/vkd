set(LIBVKD_COMPUTE_SOURCE
    bilateral.cpp
    cdl.cpp
    colour_squares.cpp
    constant.cpp
    crop.cpp
    custom.cpp
    exposure.cpp
    gaussian.cpp
    invert.cpp
    kernel.cpp
    log_exp_image.cpp
    median.cpp
    merge.cpp
    particles.cpp
    rotate.cpp
    sand.cpp
    saturation.cpp
    single_kernel.cpp
    whitebalance.cpp
)

target_sources(vkd PRIVATE ${LIBVKD_COMPUTE_SOURCE})