//
// Created by Obi Davis on 16/07/2024.
//

#ifndef JIT_MATRIX_VIEW_V2_HPP
#define JIT_MATRIX_VIEW_V2_HPP

#include "c74_jitter.h"
#include <array>
#include <span>

namespace maxutils {
    using namespace c74::max;

    template <typename T>
    t_symbol *type_sym() {
        if constexpr (std::is_same_v<T, char>) {
            return _jit_sym_char;
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return _jit_sym_long;
        } else if constexpr (std::is_same_v<T, float>) {
            return _jit_sym_float32;
        } else if constexpr (std::is_same_v<T, double>) {
            return _jit_sym_float64;
        } else {
            static_assert(sizeof(T) == 0, "Unsupported type");
            __builtin_unreachable();
        }
    }


    template <typename T>
    class row_view;

    template <typename T>
    class matrix_view;

    template <typename T>
    class cell_view {
    public:
        T &operator[](size_t i) {
            assert(i < planecount);
            return data[i];
        }
        T operator[](size_t i) const {
            assert(i < planecount);
            return data[i];
        }

        cell_view &operator=(const cell_view &other) {
            for (size_t i = 0; i < planecount; ++i) {
                data[i] = other.data[i];
            }
            return *this;
        }

        cell_view &operator=(const T &value) {
            for (size_t i = 0; i < planecount; ++i) {
                data[i] = value;
            }
            return *this;
        }

    private:
        cell_view(T *data, long planecount)
            : data{data}, planecount{planecount} {
        }
        friend class row_view<T>;
        friend class matrix_view<T>;
        T *data;
        long planecount;
    };

    template <typename T>
    class row_view {
    public:
        cell_view<T> operator[](size_t i) {
            return {data + i * planecount, planecount};
        }

        cell_view<T> operator[](size_t i) const {
            return {data + i * planecount, planecount};
        }

        struct iterator {
            T *data;
            long planecount;

            cell_view<T> operator*() {
                return {data, planecount};
            }

            iterator &operator++() {
                data += planecount;
                return *this;
            }

            iterator operator++(int) {
                iterator it = *this;
                ++(*this);
                return it;
            }

            bool operator==(const iterator &other) const {
                return data == other.data;
            }

            bool operator!=(const iterator &other) const {
                return data != other.data;
            }
        };

        iterator begin() {
            return {data, planecount};
        }

        iterator end() {
            return {data + (dim * planecount), planecount};
        }

        auto as_1d_span() const {
            return std::span(data, dim * planecount);
        }
    private:
        row_view(T *data, long planecount, long dim)
            : data{data}, planecount{planecount}, dim{dim} {
        }

        friend class matrix_view<T>;

        T *data;
        long planecount;
        long dim;
    };

    template <typename T>
    class matrix_view {
    public:
        explicit matrix_view(t_object *matrix) : info{}, data{}, matrix{matrix} {
            jit_object_method(matrix, _jit_sym_getdata, &data);
            if (data == nullptr) {
                throw std::runtime_error("Invalid data");
            }
            jit_object_method(matrix, _jit_sym_getinfo, &info);
            if (info.type != type_sym<T>()) {
                throw std::runtime_error("Type mismatch");
            }
        }

        row_view<T> row(size_t i) {
            return {
                reinterpret_cast<T *>(data + i * info.dimstride[1]),
                info.planecount,
                info.dim[0]
            };
        }

        cell_view<T> at(std::integral auto ...indices) {
            assert(sizeof...(indices) == info.dimcount);
            long offset = 0;
            long indices_arr[] = {static_cast<long>(indices)...};
            for (size_t i = 0; i < info.dimcount; ++i) {
                assert(indices_arr[i] < info.dim[i] && indices_arr[i] >= 0);
                offset += indices_arr[i] * info.dimstride[i];
            }
            assert(offset <= info.size);
            return {reinterpret_cast<T *>(data + offset), info.planecount};
        }

        [[nodiscard]] size_t planecount() const {
            return info.planecount;
        }

        [[nodiscard]] long dim(long i) const {
            return info.dim[i];
        }

        [[nodiscard]] long nrows() const {
            return dim(1);
        }

        [[nodiscard]] long ncols() const {
            return dim(0);
        }

        template <size_t N>
        t_jit_err set_dims(const long (&dims)[N]) {
            for (size_t i = 0; i < N; ++i) {
                info.dim[i] = dims[i];
            }
            info.dimcount = N;

            auto err = (t_jit_err)jit_object_method(matrix, _jit_sym_setinfo_ex, &info);
            if (err) {
                data = nullptr;
                return err;
            }

            err = (t_jit_err)jit_object_method(matrix, _jit_sym_getinfo, &info);
            if (err) {
                data = nullptr;
                return err;
            }

            err = (t_jit_err)jit_object_method(matrix, _jit_sym_getdata, &data);
            return err;
        }

        t_jit_err set_planecount(long planecount) {
            info.planecount = planecount;
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
        t_jit_matrix_info info;
        char *data;
        t_object *matrix;
    };


}

#endif //JIT_MATRIX_VIEW_V2_HPP
