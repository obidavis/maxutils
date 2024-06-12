//
// Created by Obi Davis on 24/05/2024.
//

#ifndef REFLECT_HPP
#define REFLECT_HPP

#include <string_view>
#include <type_traits>

namespace maxutils::detail {

        template<auto V>
        constexpr static auto n() noexcept {
#ifdef _MSC_VER
            std::string_view sv = __FUNCSIG__;
            auto to = sv.rfind('>')-1;
            for (std::size_t close = sv[to] == ')'; close > 0; ) {
                switch(sv[to = sv.find_last_of(")(", to-1)]) {
                    case ')': ++close; break;
                    case '(': if (!--close) --to; break;
                }
            }
            for (std::size_t close = sv[to] == '>'; close > 0; ) {
                switch(sv[to = sv.find_last_of("><", to-1)]) {
                    case '>': ++close; break;
                    case '<': if (!--close) --to; break;
                }
            }
            auto from = sv.find_last_of(">:", to);
            return sv.substr(from + 1, to - from);
#else
            std::string_view sv = __PRETTY_FUNCTION__;
            auto from = sv.rfind(':');
            return sv.substr(from + 1, sv.size() - 2 - from);
#endif
        }

        template<class From, class Type>
        From get_base_type(Type From::*);

        template<class T>
        union union_type {
            char c;
            T f;

            constexpr union_type() : c{} {
            }
        };

        template<class T>
        constexpr extern T constexpr_static_init{};

        template<auto V>
        constexpr static std::string_view get_name_if_exists() noexcept {
            if constexpr (std::is_member_function_pointer_v<decltype(V)>) {
                return n<V>();
            } else {
#ifdef _MSC_VER
#  if _MSVC_LANG >= 202000
                return n<&(constexpr_static_init<
                    union_type<decltype(get_base_type(V))>
                >.f.*V)>();
#  else // if_MSVC_LANG < 202000
                return "";
#  endif // _MSVC_LANG >= 202000
#else // if !defined(_MSC_VER)
                return n<V>();
#endif // _MSC_VER
            }
        }

        template<auto N>
        constexpr auto get_name(std::string_view name = get_name_if_exists<N>()) {
            return name;
        }
}

#endif //REFLECT_HPP
