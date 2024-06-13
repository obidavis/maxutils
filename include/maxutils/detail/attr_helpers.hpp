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

    template <typename ObjT, typename ValueT, auto Member>
    struct default_accessors;

}

#endif //ATTR_HELPERS_HPP
