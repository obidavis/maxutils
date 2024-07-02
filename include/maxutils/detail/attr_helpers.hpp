//
// Created by Obi Davis on 24/05/2024.
//

#ifndef ATTR_HELPERS_HPP
#define ATTR_HELPERS_HPP

#include "max_types.h"

namespace maxutils::detail {
    template<typename T>
    t_symbol *type_to_symbol() {
        if constexpr (std::is_same_v<std::remove_all_extents_t<T>, long>) {
            return gensym("long");
        } else if constexpr (std::is_same_v<std::remove_all_extents_t<T>, float>) {
            return gensym("float32");
        } else if constexpr (std::is_same_v<std::remove_all_extents_t<T>, double>) {
            return gensym("float64");
        } else if constexpr (std::is_same_v<std::remove_all_extents_t<T>, t_symbol *>) {
            return gensym("symbol");
        } else if constexpr (sizeof(std::remove_all_extents_t<T>) == 1) {
            return gensym("char");
        } else if constexpr (std::is_enum_v<T>) {
            return gensym("symbol");
        } else {
            static_assert(sizeof(T) == 0, "Unsupported type");
            __builtin_unreachable();
        }
    }

    template <typename T>
    static constexpr auto atom_get(t_atom *atom) {
        if constexpr (std::is_integral_v<T>) {
            return atom_getlong(atom);
        } else if constexpr (std::is_floating_point_v<T>) {
            return atom_getfloat(atom);
        } else if constexpr (std::is_same_v<T, t_symbol *>) {
            return atom_getsym(atom);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return std::string(atom_getsym(atom)->s_name);
        } else {
            static_assert(sizeof(T) == 0, "Unsupported type");
            __builtin_unreachable();
        }
    }

    template <typename T>
    static void atom_set(t_atom *atom, T value) {
        if constexpr (std::is_arithmetic_v<T>) {
            atom_setfloat(atom, value);
        } else if constexpr (std::is_floating_point_v<T>) {
            atom_setfloat(atom, value);
        } else if constexpr (std::is_same_v<T, t_symbol *>) {
            atom_setsym(atom, value);
        } else if constexpr (std::is_same_v<T, std::string>) {
            atom_setsym(atom, gensym(value.c_str()));
        } else {
            static_assert(sizeof(T) == 0, "Unsupported type");
            __builtin_unreachable();
        }
    }

}

#endif //ATTR_HELPERS_HPP
