
set(TINYXML2_SOURCES
    tinyxml2.cpp)

add_library(tinyxml2 STATIC ${TINYXML2_SOURCES})
target_include_directories(tinyxml2 PUBLIC ./)
