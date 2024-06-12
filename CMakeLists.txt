add_library(utils INTERFACE attr.hpp jit_matrix_helpers.hpp
        detail/attr_helpers.hpp
        detail/reflection.hpp
        detail/fixed_string.hpp
        detail/concepts.hpp
        detail/member_pointer.hpp
        detail/functors.hpp)
target_include_directories(utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(utils INTERFACE ${MAX_SDK_BASE}/c74support/max-includes)
target_include_directories(utils INTERFACE ${MAX_SDK_BASE}/c74support/jit-includes)
