//
// Created by Obi Davis on 21/03/2024.
//

#ifndef MATRIX_HPP
#define MATRIX_HPP

#include <utility>
#include <tuple>
#include <concepts>
#include "c74_jitter.h"

namespace maxutils {
    using namespace c74::max;

    class JitMopIO {
        t_jit_object *ob;

        explicit JitMopIO(t_jit_object *ob): ob{ob} {
        }

    public:
        static JitMopIO get_input(t_jit_object *mop, long index) {
            return JitMopIO((t_jit_object *) jit_object_method(mop, _jit_sym_getinput, index));
        }

        static JitMopIO get_output(t_jit_object *mop, long index) {
            return JitMopIO((t_jit_object *) jit_object_method(mop, _jit_sym_getoutput, index));
        }

        JitMopIO &restrict_dimcount_to(long dimcount) {
            jit_attr_setlong(ob, _jit_sym_mindimcount, dimcount);
            jit_attr_setlong(ob, _jit_sym_maxdimcount, dimcount);
            return *this;
        }

        JitMopIO &restict_dimcount_between(long min, long max) {
            jit_attr_setlong(ob, _jit_sym_mindimcount, min);
            jit_attr_setlong(ob, _jit_sym_maxdimcount, max);
            return *this;
        }

        JitMopIO &restrict_planecount_to(long planecount) {
            jit_attr_setlong(ob, _jit_sym_minplanecount, planecount);
            jit_attr_setlong(ob, _jit_sym_maxplanecount, planecount);
            return *this;
        }

        JitMopIO &restrict_planecount_between(long min, long max) {
            jit_attr_setlong(ob, _jit_sym_minplanecount, min);
            jit_attr_setlong(ob, _jit_sym_maxplanecount, max);
            return *this;
        }

        JitMopIO &restrict_types_to(t_symbol *type) {
            jit_attr_setsym(ob, _jit_sym_types, type);
            return *this;
        }

        JitMopIO &restrict_types_to(t_symbol *type, std::same_as<t_symbol *> auto... types) {
            t_symbol *types_array[] = {type, types...};
            jit_attr_setsym_array(ob, _jit_sym_types, 1 + sizeof...(types), types_array);
            return *this;
        }

        JitMopIO &no_dimlink() {
            jit_attr_setlong(ob, _jit_sym_dimlink, 0);
            return *this;
        }

        JitMopIO &no_planelink() {
            jit_attr_setlong(ob, _jit_sym_planelink, 0);
            return *this;
        }

        JitMopIO &no_typelink() {
            jit_attr_setlong(ob, _jit_sym_typelink, 0);
            return *this;
        }

        JitMopIO &no_link() {
            no_dimlink();
            no_planelink();
            no_typelink();
            return *this;
        }

        JitMopIO &use_ioproc_custom_fn(t_jit_err (*fn)(void *, void *, void *)) {
            jit_object_method(ob, _jit_sym_ioproc, fn);
            return *this;
        }

        JitMopIO &use_ioproc_copy_adapt() {
            return use_ioproc_custom_fn(jit_mop_ioproc_copy_adapt);
        }

        JitMopIO &use_iopproc_copy_trunc() {
            return use_ioproc_custom_fn(jit_mop_ioproc_copy_trunc);
        }

        JitMopIO &use_ioproc_copy_trunc_zero() {
            return use_ioproc_custom_fn(jit_mop_ioproc_copy_trunc_zero);
        }
    };

    template <size_t N>
    class MatrixLock {
        std::pair<t_object *, long> matrices_with_locks[N];

    public:
        template <std::same_as<t_object *> ...Ts>
        requires(sizeof...(Ts) > 0)
        explicit MatrixLock(Ts... matrices)
            : matrices_with_locks{std::make_pair(matrices, (long) jit_object_method(matrices, _jit_sym_lock, 1))...} {
        }

        ~MatrixLock() {
            for (auto [matrix, lock]: matrices_with_locks) {
                jit_object_method(matrix, _jit_sym_lock, lock);
            }
        }
    };

    template <std::same_as<t_object *> ...Ts>
    MatrixLock(Ts ...matrices) -> MatrixLock<sizeof...(Ts)>;

    template<std::convertible_to<bool> ...Ts>
    bool matrix_pointers_are_valid(Ts... args) {
        return (args && ...);
    }

    inline auto get_matrix_object_from_list(void *list) {
        return (t_object *) jit_object_method(list, _jit_sym_getindex, 0);
    }

    template<size_t I>
    auto get_matrix_objects_from_list(void *list) {
        return [list]<long ... Is>(std::integer_sequence<long, Is...>) {
            return std::make_tuple((t_object *) jit_object_method(list, _jit_sym_getindex, Is)...);
        }(std::make_integer_sequence<long, I>{});
    }

    t_jit_err jit_matrix_set_type(t_object *matrix, t_symbol *type) {
        t_jit_matrix_info info;
        jit_object_method(matrix, _jit_sym_getinfo, &info);
        info.type = type;
        return (t_jit_err) jit_object_method(matrix, _jit_sym_setinfo_ex, &info);
    }

    t_jit_err jit_matrix_set_dims(t_object *matrix, long dim, std::convertible_to<long> auto... dims) {
        static_assert(sizeof...(dims) < JIT_MATRIX_MAX_DIMCOUNT - 1, "Too many dimensions.");

        if (!matrix) {
            return JIT_ERR_INVALID_PTR;
        }

        t_jit_matrix_info info;
        auto err = (t_jit_err) jit_object_method(matrix, _jit_sym_getinfo, &info);
        if (err) {
            return err;
        }

        info.dimcount = 1 + sizeof...(dims);
        info.dim[0] = dim;
        long i = 1;
        for (long d: {dims...}) {
            info.dim[i++] = std::max(1l, d);
        }

        return (t_jit_err) jit_object_method(matrix, _jit_sym_setinfo, &info);
    }
}

#endif //MATRIX_HPP
