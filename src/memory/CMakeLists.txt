set(LIBVKD_memory_SOURCE
    memory_manager.cpp
    memory_pool.cpp
)

target_sources(vkd PRIVATE ${LIBVKD_memory_SOURCE})