include(${MAX_SDK_BASE}/script/max-pretarget.cmake)

project(test_attr)

add_library(test_attr
    MODULE
        test_attr.cpp)

target_include_directories(test_attr PRIVATE ${C74_INCLUDES})
target_link_libraries(test_attr PRIVATE maxutils)
target_compile_features(test_attr PRIVATE cxx_std_20)

include(${MAX_SDK_BASE}/script/max-posttarget.cmake)
