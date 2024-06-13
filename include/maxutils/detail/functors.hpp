//
// Created by Obi Davis on 24/05/2024.
//

#ifndef FUNCTORS_HPP
#define FUNCTORS_HPP

#include <type_traits>
#include <tuple>

namespace maxutils::detail {
    template <typename T>
    struct function_traits;

    template <typename Ret, typename ...Args>
    struct function_traits<Ret(*)(Args...)> {
        using return_type = Ret;
        using arg_types = std::tuple<Args...>;
    };

    template <typename T>
    struct lambda_traits : lambda_traits<decltype(&T::operator())> {};

    template <typename Class, typename Ret, typename ...Args>
    struct lambda_traits<Ret(Class::*)(Args...) const> {
        using return_type = Ret;
        using arg_types = std::tuple<Args...>;
    };

    template <typename T>
    struct functor_traits : std::conditional_t<
        std::is_function_v<std::remove_pointer_t<T>>,
        function_traits<T>,
        lambda_traits<T>
    > {};

    template <typename T>
    using return_type_t = typename functor_traits<T>::return_type;

    template <typename T>
    using arg_types_t = typename functor_traits<T>::arg_types;

    template <typename T, size_t N>
    using nth_arg_type_t = std::tuple_element_t<N, arg_types_t<T>>;
}

#endif //FUNCTORS_HPP
