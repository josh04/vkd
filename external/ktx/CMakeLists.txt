

set(KTX_SOURCES
    lib/texture.c
    lib/hashlist.c
    lib/checkheader.c
    lib/swap.c
    lib/memstream.c
    lib/filestream.c)

add_library(ktx STATIC ${KTX_SOURCES})

target_include_directories(ktx PUBLIC include ../../include)
