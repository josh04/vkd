set(TEST_SOURCE
    load_save.cpp
)

add_executable(vkd-test ${TEST_SOURCE})
target_link_libraries(vkd-test PUBLIC vkd)
