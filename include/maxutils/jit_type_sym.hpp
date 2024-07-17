//
// Created by Obi Davis on 05/07/2024.
//

#ifndef JIT_TYPE_SYM_HPP
#define JIT_TYPE_SYM_HPP

#include <cstdint>
#include <jit.common.h>

namespace maxutils {
    using namespace c74::max;
    template <typename T>
    struct type_sym;

    template <>
    struct type_sym<char> { static t_symbol *value() { return _jit_sym_char; } };

    template <>
    struct type_sym<int32_t> { static t_symbol *value() { return _jit_sym_long; } };

    template <>
    struct type_sym<float> { static t_symbol *value() { return _jit_sym_float32; } };

    template <>
    struct type_sym<double> { static t_symbol *value() { return _jit_sym_float64; } };

}
#endif //JIT_TYPE_SYM_HPP
