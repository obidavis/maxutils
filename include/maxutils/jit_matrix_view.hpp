//
// Created by Obi Davis on 05/07/2024.
//

#ifndef JIT_MATRIX_VIEW_HPP
#define JIT_MATRIX_VIEW_HPP

#include "c74_jitter.h"
#include "jit_type_sym.hpp"
#include <span>
#include <cassert>

namespace maxutils {
    using namespace c74::max;

    template <typename T, size_t N>
    struct vec_ {
        T data[N];
        T &operator[](size_t i) { return data[i]; }
        T operator[](size_t i) const { return data[i]; }
    };

    using vec2f = vec_<float, 2>;
    using vec3f = vec_<float, 3>;
    using vec4f = vec_<float, 4>;

    template <typename T>
    struct extent;

    template <>
    struct extent<char> { static constexpr size_t value = 1; };

    template <>
    struct extent<int32_t> { static constexpr size_t value = 1; };

    template <>
    struct extent<float> { static constexpr size_t value = 1; };

    template <>
    struct extent<double> { static constexpr size_t value = 1; };

    template <typename T, size_t N>
    struct extent<vec_<T, N>> { static constexpr size_t value = N; };

    template <typename T>
    static constexpr size_t extent_v = extent<T>::value;

    template <typename T, size_t N>
    struct type_sym<vec_<T, N>> { static t_symbol *value() { return type_sym<T>::value(); } };

    class jit_matrix_view {
    public:
        explicit jit_matrix_view(t_jit_object *matrix) : matrix{matrix} {
            jit_object_method(matrix, _jit_sym_getinfo, &info);
            jit_object_method(matrix, _jit_sym_getdata, &data);
        }

        template <typename T>
        std::span<T> row(size_t i) {
            assert(info.type == type_sym<T>::value());
            assert(info.planecount == extent_v<T>);
            assert(info.dimcount == 2);
            assert(data != nullptr);
            T *p = reinterpret_cast<T *>(data + i * info.dimstride[1]);
            return {p, static_cast<size_t>(info.dim[0])};
        }

        template <typename T>
        T &at(std::convertible_to<long> auto ...indices) {
            assert(info.type == type_sym<T>::value());
            assert(info.planecount == extent_v<T>);
            // assert(sizeof...(indices) == info.dimcount);
            assert(data != nullptr);

            long offset = 0;
            long indices_arr[] = {static_cast<long>(indices)...};
            for (size_t i = 0; i < info.dimcount; ++i) {
                offset += indices_arr[i] * info.dimstride[i];
            }
            assert(offset <= info.size);
            return *reinterpret_cast<T *>(data + offset);
        }

        size_t cols() const {
            return info.dim[0];
        }

        size_t rows() const {
            return info.dim[1];
        }

        size_t dim(size_t n) const {
            assert(n < JIT_MATRIX_MAX_DIMCOUNT);
            return info.dim[n];
        }

        size_t planecount() const {
            return info.planecount;
        }

        t_jit_err set_dims(std::convertible_to<long> auto ...dims) {
            int i = 0;
            for (auto dim : {dims...}) {
                info.dim[i++] = dim;
            }
            info.dimcount = i;
            auto err = (t_jit_err)jit_object_method(matrix, _jit_sym_setinfo_ex, &info);
            if (err != JIT_ERR_NONE) {
                data = nullptr;
                return err;
            }
            err = (t_jit_err)jit_object_method(matrix, _jit_sym_getinfo, &info);
            if (err != JIT_ERR_NONE) {
                data = nullptr;
                return err;
            }
            err = (t_jit_err)jit_object_method(matrix, _jit_sym_getdata, &data);
            return err;
        }

        t_jit_err clear() {
            return (t_jit_err)jit_object_method(matrix, _jit_sym_clear);
        }
    private:
        t_jit_object *matrix;
        t_jit_matrix_info info;
        char *data;
    };
}

#endif //JIT_MATRIX_VIEW_HPP
