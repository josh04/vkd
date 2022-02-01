set(TEST_SOURCE
    load_save.cpp
    test_ocio.cpp
)

add_executable(vkd-test ${TEST_SOURCE})
target_link_libraries(vkd-test PUBLIC vkd)

if(WIN32)
set_target_properties(vkd-test PROPERTIES LINK_FLAGS "/wd4251")
endif()

if (CMAKE_BUILD_TYPE MATCHES Debug)
    set(BUNDLE_DIR debug)
else()
    set(BUNDLE_DIR release)
endif()

install(TARGETS vkd-test DESTINATION bin)

if(APPLE)
    install(CODE "
      
        execute_process(COMMAND \"/usr/bin/install_name_tool\" \"-add_rpath\" \"@executable_path/\" \"${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/bin/vkd-test\")
        execute_process(COMMAND \"/usr/bin/install_name_tool\" \"-add_rpath\" \"@executable_path/../lib\" \"${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/bin/vkd-test\")
        execute_process(COMMAND \"/usr/bin/install_name_tool\" \"-change\" \"libglslang.11.dylib\" \"@rpath/libglslang.11.dylib\" \"${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/bin/vkd-test\")
        execute_process(COMMAND \"/usr/bin/install_name_tool\" \"-change\" \"libglslang.11.dylib\" \"@rpath/libglslang.11.dylib\" \"${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/lib/libvkd.dylib\")
        execute_process(COMMAND \"/usr/bin/install_name_tool\" \"-change\" \"libSPIRV.dylib\" \"@rpath/libSPIRV.dylib\" \"${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/bin/vkd-test\")
        execute_process(COMMAND \"/usr/bin/install_name_tool\" \"-change\" \"libSPIRV.dylib\" \"@rpath/libSPIRV.dylib\" \"${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/lib/libvkd.dylib\")

        #execute_process(COMMAND \"codesign --sign - ${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/bin/spacetrain.app\")

       " COMPONENT Bundle)
endif(APPLE)


