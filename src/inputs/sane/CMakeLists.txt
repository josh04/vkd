set(LIBVKD_sane_SOURCE
    sane_wrapper.cpp
    sane_service.cpp
)

target_sources(vkd PRIVATE ${LIBVKD_sane_SOURCE})