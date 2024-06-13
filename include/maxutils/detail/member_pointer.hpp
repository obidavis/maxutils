//
// Created by Obi Davis on 24/05/2024.
//

#ifndef MEMBER_POINTER_HPP
#define MEMBER_POINTER_HPP

#include <cstddef>

namespace maxutils::detail {
    template<auto Member>
    struct member_pointer_info;

    template<typename Obj, typename T, T Obj::* Member>
    struct member_pointer_info<Member> {
        using ObjectType = Obj;
        using ValueType = T;

        constexpr static size_t offset() {
            return reinterpret_cast<size_t>(&((Obj *) nullptr->*Member));
        }
    };

    template <auto Member>
    using member_pointer_object_type_t = typename member_pointer_info<Member>::ObjectType;

    template <auto Member>
    using member_pointer_value_type_t = typename member_pointer_info<Member>::ValueType;

}

#endif //MEMBER_POINTER_HPP
