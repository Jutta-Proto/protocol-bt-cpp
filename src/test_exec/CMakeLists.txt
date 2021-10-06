cmake_minimum_required(VERSION 3.16)

set(EXECUTABLE_NAME "test_exec")
set(EXECUTABLE_MAIN "test_exec.cpp")

add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_MAIN})
target_link_libraries(${EXECUTABLE_NAME} PRIVATE logger bt jutta_bt_proto)
set_property(SOURCE ${EXECUTABLE_MAIN} PROPERTY COMPILE_DEFINITIONS)
