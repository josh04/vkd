set(TEXTURES 
    particle_fire.ktx
    particle_gradient_rgba.ktx
    particle_smoke.ktx
    particle01_rgba.ktx
)

add_custom_target(copy_tex WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
        ${TEXTURES}
        ${CMAKE_BINARY_DIR}/textures
)