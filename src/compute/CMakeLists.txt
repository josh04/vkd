set(LIBVKD_COMPUTE_SOURCE
    bilateral.cpp
    gaussian.cpp
    colour_squares.cpp
    exposure.cpp
    kernel.cpp
    merge.cpp
    particles.cpp
    sand.cpp
)

target_sources(vkd PRIVATE ${LIBVKD_COMPUTE_SOURCE})