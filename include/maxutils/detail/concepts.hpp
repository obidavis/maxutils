//
// Created by Obi Davis on 24/05/2024.
//

#ifndef CONCEPTS_HPP
#define CONCEPTS_HPP

#include <type_traits>

namespace maxutils {
    template<typename T>
    concept NumericAttr = std::is_same_v<T, long> || std::is_same_v<T, float> || std::is_same_v<T, double> || std::is_same_v<T, char> || std::is_same_v<T, unsigned char>;

    template<typename T>
    concept BooleanAttr = std::is_same_v<T, bool>;

    template<typename T>
    concept EnumAttr = std::is_enum_v<T>;



    template <typename T>
    concept ArrayAttr = std::is_array_v<T> && NumericAttr<std::remove_all_extents_t<T>>;

    template<typename T>
    concept Attr = NumericAttr<T> || BooleanAttr<T> || EnumAttr<T> || ArrayAttr<T>;

    template<typename T, typename U>
    concept PurePredicate = std::predicate<T, U> && requires(T t, U u)
    {
        { +t } -> std::convertible_to<bool (*)(U)>; // Unary plus operator
    };

}

#endif //CONCEPTS_HPP
