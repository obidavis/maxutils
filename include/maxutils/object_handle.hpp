//
// Created by Obi Davis on 20/06/2024.
//

#ifndef OBJECT_HANDLE_HPP
#define OBJECT_HANDLE_HPP

#include <memory>
#include "ext.h"

namespace maxutils {

    template <typename T>
    concept PtrIntComaptible = sizeof(T) == sizeof(t_ptr_int);

    t_object *object_new_defaults(
        void * a1 = nullptr,
        void * a2 = nullptr,
        void * a3 = nullptr,
        void * a4 = nullptr,
        void * a5 = nullptr,
        void * a6 = nullptr,
        void * a7 = nullptr,
        void * a8 = nullptr,
        void * a9 = nullptr,
        void * a10 = nullptr
        ) {
        return (t_object *) object_new_imp(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    }

    t_object *v_object_new(t_symbol *name, auto && ...args) {
        return object_new_defaults(name, (void *) (t_ptr_int) std::forward<decltype(args)>(args)...);
    }

    t_jit_object *jit_object_new_defaults(
        void * a1 = nullptr,
        void * a2 = nullptr,
        void * a3 = nullptr,
        void * a4 = nullptr,
        void * a5 = nullptr,
        void * a6 = nullptr,
        void * a7 = nullptr,
        void * a8 = nullptr,
        void * a9 = nullptr,
        void * a10 = nullptr
        ) {
        return (t_jit_object *) jit_object_new_imp(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    }

    t_jit_object *v_jit_object_new(t_symbol *name, auto && ...args) {
        return jit_object_new_defaults(name, (void *) (t_ptr_int) std::forward<decltype(args)>(args)...);
    }

    class object_handle {
        std::unique_ptr<t_object, decltype([](t_object *p) {
                // post("object destructing %p", p);
            object_free(p);
            })> ptr;
    public:
        object_handle(t_symbol *name, auto && ...args) : ptr{v_object_new(name, std::forward<decltype(args)>(args)...)} {
            // post("object constructing %p", ptr.get());
        }
        operator t_object *() const { return ptr.get(); }
        operator t_object *() { return ptr.get(); }
    };

    struct jit_object_deleter {
        void operator()(t_jit_object *p) const {
            assert(p != nullptr);
            // post("jit destructing %p", p);
            jit_object_free(p);
        }
    };

    class jit_object_handle {
        std::unique_ptr<t_jit_object, jit_object_deleter> ptr;
    public:
        jit_object_handle(t_symbol *name, auto && ...args) : ptr{v_jit_object_new(name, std::forward<decltype(args)>(args)...)} {
        }
        // jit_object_handle(t_jit_object *p) : ptr{p} {}
        operator t_jit_object *() const { return ptr.get(); }
        operator t_jit_object *() { return ptr.get(); }
    };

    enum class outlet_type {
        bang,
        int_,
        float_,
    };

    struct outlet_deleter {
        void operator()(t_outlet *p) const {
            assert(p != nullptr);
            outlet_delete(p);
        }
    };

    class outlet_handle {
        std::unique_ptr<t_outlet, outlet_deleter> ptr;
    public:
        outlet_handle(t_object *owner, const char *s = nullptr) : ptr{outlet_new(owner, s)} {}
        outlet_handle(t_object *owner, outlet_type type) {
            switch (type) {
                case outlet_type::bang:
                    ptr.reset(bangout(owner));
                    break;
                case outlet_type::int_:
                    ptr.reset(intout(owner));
                    break;
                case outlet_type::float_:
                    ptr.reset(floatout(owner));
                    break;
            }
        }
        operator t_outlet *() const { return ptr.get(); }
    };
}

#endif //OBJECT_HANDLE_HPP
