cmake_minimum_required(VERSION 3.26)
project(maxutils)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")

option(MAX_SDK_BASE_PATH "Path to the Max SDK" "")
option(FETCH_MAX_SDK_FROM_GITHUB "Fetch the Max SDK from GitHub" OFF)

if (FETCH_MAX_SDK_FROM_GITHUB)
    include(FetchContent)
    message(STATUS "Fetching Max SDK Base from GitHub")
    FetchContent_Declare(
            max-sdk-base
            GIT_REPOSITORY https://github.com/Cycling74/max-sdk-base.git
            GIT_TAG main
    )
    FetchContent_MakeAvailable(max-sdk-base)
    set(MAX_SDK_BASE ${max-sdk-base_SOURCE_DIR})
endif()


add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} INTERFACE ${MAX_SDK_BASE}/c74support/max-includes)
target_include_directories(${PROJECT_NAME} INTERFACE ${MAX_SDK_BASE}/c74support/jit-includes)

include(cmake/magic_enum.cmake)
target_link_libraries(${PROJECT_NAME} INTERFACE magic_enum)

option(BUILD_TESTS "Build tests" OFF)
if (BUILD_TESTS)
    add_subdirectory(test)
endif()