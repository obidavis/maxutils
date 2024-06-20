//
// Created by Obi Davis on 13/06/2024.
//

#ifndef NAMED_MATRIX_HPP
#define NAMED_MATRIX_HPP

#include "ext.h"

using namespace c74::max;

template <typename T, size_t N>
struct vec {
    T data[N];
};

using vec3f = vec<float, 3>;

class NamedMatrix {
public:
    NamedMatrix() : matrix{nullptr} {
        atom_setsym(&name, _jit_sym_nothing);
    }

    explicit NamedMatrix(t_jit_matrix_info *info, t_symbol *name = nullptr)
        : matrix{}, name{} {
        matrix = (t_jit_object *) jit_object_new(gensym("jit_matrix"), info);
        if (!name) name = jit_symbol_unique();
        jit_object_register(matrix, name);
        atom_setsym(&this->name, name);
    }

    NamedMatrix(t_symbol *type, std::vector<long> dims, long planecount, t_symbol *name = nullptr)
        : matrix{}, name{} {
        info = {
            .type = type,
            .dimcount = static_cast<long>(dims.size()),
            .planecount = planecount,
        };
        for (size_t i = 0; i < dims.size(); ++i) {
            info.dim[i] = dims[i];
        }

        matrix = (t_jit_object *) jit_object_new(gensym("jit_matrix"), &info);
        if (!name) name = jit_symbol_unique();
        jit_object_register(matrix, name);
        atom_setsym(&this->name, name);
        update_info_and_data_ptr();
    }

    NamedMatrix(const NamedMatrix &) = delete;
    NamedMatrix &operator=(const NamedMatrix &) = delete;
    NamedMatrix(NamedMatrix &&) = delete;
    NamedMatrix &operator=(NamedMatrix &&) = delete;

    // NamedMatrix(NamedMatrix &&other) noexcept
    //     : matrix{other.matrix}, name{other.name}, lock_value{other.lock_value}, data_ptr{other.data_ptr}, info{other.info} {
    //     other.matrix = nullptr;
    //     other.data_ptr = nullptr;
    //     other.lock_value = 0;
    //     other.info = {};
    //     atom_setsym(&other.name, _jit_sym_nothing);
    // }
    //
    // NamedMatrix &operator=(NamedMatrix &&other) noexcept {
    //     if (this != &other) {
    //         if (matrix) {
    //             jit_object_unregister(matrix);
    //             jit_object_free(matrix);
    //         }
    //         matrix = other.matrix;
    //         name = other.name;
    //         lock_value = other.lock_value;
    //         data_ptr = other.data_ptr;
    //         info = other.info;
    //
    //         other.matrix = nullptr;
    //         atom_setsym(&other.name, _jit_sym_nothing);
    //         other.data_ptr = nullptr;
    //         other.lock_value = 0;
    //         other.info = {};
    //     }
    //     return *this;
    // }

    ~NamedMatrix() {
        jit_object_unregister(matrix);
        jit_object_free(matrix);
    }

    void *data() {
        if (!matrix) return nullptr;
        void *data;
        auto err = (t_jit_err) jit_object_method(matrix, gensym("getdata"), &data);
        if (err) {
            return nullptr;
        }
        return data;
    }

    template<typename T>
    T *data_as() {
        return static_cast<T *>(data());
    }

    void lock() {
        if (!matrix) return;
        lock_value = (long) jit_object_method(matrix, _jit_sym_lock, 1);
    }

    void release() {
        if (!matrix) return;
        jit_object_method(matrix, _jit_sym_lock, lock_value);
    }

    t_jit_err set_dims(std::vector<long> dims) {
        if (dims.size() != info.dimcount) return JIT_ERR_MISMATCH_DIM;
        for (size_t i = 0; i < dims.size(); ++i) {
            info.dim[i] = dims[i];
        }
        auto err = (t_jit_err) jit_object_method(matrix, _jit_sym_setinfo_ex, &info);
        update_info_and_data_ptr();
        return err;
    }

    template <typename T, std::convertible_to<long> ...Indices>
    T &at(Indices ...indices) {
        assert(sizeof...(indices) == info.dimcount);
        assert(data_ptr != nullptr);
        long offset = 0;
        long indices_arr[] = {indices...};
        for (size_t i = 0; i < info.dimcount; ++i) {
            offset += indices_arr[i] * info.dimstride[i];
        }
        return *reinterpret_cast<T *>(data_ptr + offset);
    }

    t_jit_err clear() {
        return (t_jit_err) jit_object_method(matrix, _jit_sym_clear);
    }

    t_jit_object *matrix;
    t_atom name;

private:
    void update_info_and_data_ptr() {
        jit_object_method(matrix, _jit_sym_getinfo, &info);
        jit_object_method(matrix, _jit_sym_getdata, &data_ptr);
    }
    long lock_value;
    unsigned char *data_ptr;
    t_jit_matrix_info info;
};

#endif //NAMED_MATRIX_HPP
