

set(SAGO_SOURCES
    platform_folders.cpp)

add_library(platform_folders STATIC ${SAGO_SOURCES})

target_include_directories(platform_folders PUBLIC include .)
