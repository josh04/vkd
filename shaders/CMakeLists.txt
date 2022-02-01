
set(GLSL_VALIDATOR "glslangValidator")
if (APPLE)
set(GLSL_VALIDATOR "/usr/local/bin/glslangValidator")
endif()

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "*/*.frag"
    "*/*.vert"
    "*/*.comp"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  file(RELATIVE_PATH DIRECT ${CMAKE_CURRENT_SOURCE_DIR} ${GLSL})

  get_filename_component(FILE_NAME ${GLSL} NAME)
  get_filename_component(DIR_NAME ${DIRECT} DIRECTORY)

  set(SPIRV "${CMAKE_CURRENT_BINARY_DIR}/${DIR_NAME}/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    spirv-shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/shaders DESTINATION bin)