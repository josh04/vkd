set(LIBVKD_ocio_SOURCE
    ocio.cpp
    ocio_functional.cpp
    ocio_static.cpp
)

target_sources(vkd PRIVATE ${LIBVKD_ocio_SOURCE})