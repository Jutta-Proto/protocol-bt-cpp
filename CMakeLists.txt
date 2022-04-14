cmake_minimum_required(VERSION 3.16)

set(PROJECT_DESCRIPTION "An unofficial Bluetooth protocol library implementation to control JURA coffee makers.")

project("Jutta Bluteooth Protocol Library" 
            VERSION 0.0.0
            DESCRIPTION "${PROJECT_DESCRIPTION}"
            HOMEPAGE_URL "https://github.com/Jutta-Proto/protocol-bt-cpp")
set(VERSION_NAME "dev")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

macro(jutta_bt_proto_option OPTION_NAME OPTION_TEXT OPTION_DEFAULT)
    option(${OPTION_NAME} ${OPTION_TEXT} ${OPTION_DEFAULT})
    if(DEFINED ENV{${OPTION_NAME}})
        # Allow setting the option through an environment variable
        set(${OPTION_NAME} $ENV{${OPTION_NAME}})
    endif()
    if(${OPTION_NAME})
        add_definitions(-D${OPTION_NAME})
    endif()
    message(STATUS "  ${OPTION_NAME}: ${${OPTION_NAME}}")
endmacro()

message(STATUS "C++ Jutta Bluetooth Protocol Library Options")
message(STATUS "=======================================================")
jutta_bt_proto_option(JUTTA_BT_PROTO_BUILD_TEST_EXEC "Set to ON to build test executable." OFF)
jutta_bt_proto_option(JUTTA_BT_PROTO_BUILD_TESTS "Set to ON to build tests." OFF)
jutta_bt_proto_option(JUTTA_BT_PROTO_STATIC_ANALYZE "Set to ON to enable the GCC 10 static analysis. If enabled, JUTTA_BT_PROTO_ENABLE_LINTING has to be disabled." OFF)
jutta_bt_proto_option(JUTTA_BT_PROTO_ENABLE_LINTING "Set to ON to enable clang linting. If enabled, JUTTA_BT_PROTO_STATIC_ANALYZE has to be disabled." OFF)
message(STATUS "=======================================================")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR})

include(sanitizer)
include(gcc_analyze)
include(clear_variable)
include(FetchContent)

if(${JUTTA_BT_PROTO_ENABLE_LINTING})
    message(STATUS "Enabling linting")
    include(clang-tidy)
else()
    message(STATUS "Linting is disabled")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake"
                "${CMAKE_BINARY_DIR}/conan.cmake"
                TLS_VERIFY ON)
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

set(CONAN_CONFIGS"Release;Debug;RelWithDebInfo")
if(NOT CMAKE_BUILD_TYPE IN_LIST CONAN_CONFIGS)
    set(CONAN_BUILD_TYPE "Debug")
else()
    set(CONAN_BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif()

conan_cmake_configure(REQUIRES catch2/2.13.8
                               spdlog/1.10.0
                               tinyxml2/9.0.0
                      GENERATORS cmake_find_package
                      BUILD missing)
conan_cmake_autodetect(settings)
conan_cmake_install(PATH_OR_REFERENCE .
                    BUILD missing
                    REMOTE conancenter
                    SETTINGS ${settings})

find_package(spdlog REQUIRED)
find_package(tinyxml2 REQUIRED)

# Disable linting for fetch content projects
clear_variable(DESTINATION CMAKE_CXX_CLANG_TIDY BACKUP CMAKE_CXX_CLANG_TIDY_BKP)

set(GATTLIB_BUILD_DOCS OFF)
FetchContent_Declare(gattlib
                     GIT_REPOSITORY https://github.com/labapart/gattlib.git
                     GIT_TAG bb90b55ddec9c1817729a507ca096c2a514d8dc3) # State on master from the 03.12.2021
FetchContent_MakeAvailable(gattlib)

FetchContent_Declare(eventpp
                     GIT_REPOSITORY https://github.com/wqking/eventpp.git
                     GIT_TAG cf1ba5689d51d9aafeedfc28788f08690d8b0f40) # State on master from the 18.01.2022
FetchContent_MakeAvailable(eventpp)

restore_variable(DESTINATION CMAKE_CXX_CLANG_TIDY BACKUP CMAKE_CXX_CLANG_TIDY_BKP)

include_directories(${CMAKE_SOURCE_DIR}/src)

add_subdirectory(src)

# Testing
if(${JUTTA_BT_PROTO_BUILD_TESTS})
    message(STATUS "Testing is enabled")
    enable_testing()
    add_subdirectory(tests)
else()
    message(STATUS "Testing is disabled")
endif()

