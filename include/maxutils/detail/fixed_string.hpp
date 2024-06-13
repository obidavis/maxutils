//
// Created by Obi Davis on 24/05/2024.
//

#ifndef FIXED_STRING_HPP
#define FIXED_STRING_HPP

#include <array>
#include <cstddef>

namespace maxutils::detail {

    template<std::size_t N>
    struct fixed_string {
        std::array<char, N> data;

        constexpr fixed_string(const char (&str)[N]) noexcept {
            for (std::size_t i = 0; i < N; ++i) {
                data[i] = str[i];
            }
        }

        [[nodiscard]] constexpr const char *c_str() const noexcept {
            return data.data();
        }

        [[nodiscard]] constexpr std::size_t size() const noexcept {
            return N;
        }

        operator const char *() const noexcept {
            return c_str();
        }
    };

    template<std::size_t N>
    fixed_string(const char (&)[N]) -> fixed_string<N>;

}

#endif //FIXED_STRING_HPP
