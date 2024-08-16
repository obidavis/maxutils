//
// Created by Obi Davis on 25/06/2024.
//

#ifndef ATTRIBUTES_HPP
#define ATTRIBUTES_HPP

#include <type_traits>
#include "detail/attr_helpers.hpp"
#include "detail/member_pointer.hpp"
#include "detail/functors.hpp"
#include "detail/reflection.hpp"
#include "magic_enum.hpp"
#include "ext_obex.h"

namespace maxutils {
    using namespace c74::max;

    using err_t = t_atom_long;
    namespace detail {
        template <auto member_ptr>
        struct offset_default_accessors {
            using object_t = member_pointer_object_type_t<member_ptr>;
            using value_t = member_pointer_value_type_t<member_ptr>;

        };

        template <typename T, typename Object>
        concept EffectFn = requires(T &&fn, Object *x) {
            { std::invoke(fn, x) } -> std::same_as<err_t>;
        };

        template <typename T, typename Object, typename Value>
        concept GetterFn = requires(T &&fn, Object *x) {
            { std::invoke(fn, x) } -> std::same_as<Value>;
        };

        template <typename T, typename Object, typename Value>
        concept SetterFn = requires(T &&fn, Object *x, Value v) {
            { std::invoke(fn, x, v) } -> std::same_as<err_t>;
        };

        template <typename Derived>
        class attr_builder {
        public:
            Derived &with_label(const std::string &label) {
                class_attr_addattr_format(c, name.c_str(), "label", gensym("symbol"), 0, "s", gensym_tr(label.c_str()));
                return static_cast<Derived &>(*this);
            }
            Derived &readonly() {
                long old_flags = (long)object_method(attr, gensym("getflags"));
                object_method(attr, gensym("setflags"), old_flags | ATTR_SET_OPAQUE);
                return static_cast<Derived &>(*this);
            }
            Derived &user_readonly() {
                long old_flags = (long)object_method(attr, gensym("getflags"));
                object_method(attr, gensym("setflags"), old_flags | ATTR_SET_OPAQUE_USER);
                return static_cast<Derived &>(*this);
            }
        protected:
            attr_builder(t_class *c, std::string name, t_symbol *type, long offset = 0)
                : c{c}, name{name}, type{type} {
                attr = attr_offset_new(name.c_str(), type, 0, nullptr, nullptr, offset);
                class_addattr(c, attr);
                with_label(auto_format_label());
            }
            std::string auto_format_label() const {
                std::string label = name;
                for (char & i : label) {
                    if (i == '_') {
                        i = ' ';
                    }
                }
                for (size_t i = 0; i < label.size(); i++) {
                    if (i == 0 || label[i - 1] == ' ') {
                        label[i] = std::toupper(label[i]);
                    }
                }
                return label;
            }

            t_class *c;
            std::string name;
            t_symbol *type;
            t_object *attr;
        };

        template <auto member_ptr>
        class offset_attr_builder : public attr_builder<offset_attr_builder<member_ptr>> {
            using object_t = member_pointer_object_type_t<member_ptr>;
            using value_t = member_pointer_value_type_t<member_ptr>;
        public:
            offset_attr_builder(t_class *c, std::string name);
            // Generic options
            offset_attr_builder &with_effect(EffectFn<object_t> auto &&effect) {
                // cache the effect function
                static const auto s_effect = effect;
                auto setter_with_effect = [](object_t *x, void *attr, long argc, t_atom *argv) {
                    auto err = setter(x, attr, argc, argv);
                    if (err == 0) {
                        return std::invoke(s_effect, x);
                    } else {
                        return err;
                    }
                };
                object_method(this->attr, gensym("setmethod"), gensym("set"), (method) +setter_with_effect);
                return *this;
            }

            // Numeric specific attributes
            offset_attr_builder &satisfying_predicate(std::predicate<value_t> auto &&pred, const char *message)
                requires std::is_arithmetic_v<value_t> {
                static const auto s_pred = pred;
                static const auto s_message = message;

                auto setter_with_predicate = [](object_t *x, void *attr, long argc, t_atom *argv) {
                    auto value = atom_get<value_t>(argv);
                    if (std::invoke(s_pred, value)) {
                        return setter(x, attr, argc, argv);
                    } else {
                        object_error((t_object *) x, s_message);
                        return MAX_ERR_GENERIC;
                    }
                };

                object_method(this->attr, gensym("setmethod"), gensym("set"), (method) +setter_with_predicate);
                return *this;
            }
            offset_attr_builder &with_min_max(double min, double max) requires (std::is_arithmetic_v<value_t>) {
                attr_addfilter_clip(this->attr, min, max, 1, 1);
                return *this;
            }
        protected:
            static err_t getter(object_t *x, void *, long *argc, t_atom **argv) requires (!std::is_enum_v<value_t>) {
                value_t value = x->*member_ptr;
                char alloc;
                atom_alloc(argc, argv, &alloc);
                atom_set(*argv, value);
                return 0;
            }
            static err_t getter(object_t *x, void *, long *argc, t_atom **argv) requires (std::is_enum_v<value_t>) {
                value_t value = x->*member_ptr;
                std::string value_str(magic_enum::enum_name(value));
                char alloc;
                atom_alloc(argc, argv, &alloc);
                atom_setsym(*argv, gensym(value_str.c_str()));
                return 0;
            }
            static err_t setter(object_t *x, void *, long argc, t_atom *argv) requires (!std::is_enum_v<value_t>) {
                if (argc != 1) {
                    object_error((t_object *) x, "Expected 1 argument, got %ld", argc);
                    return MAX_ERR_GENERIC;
                }
                x->*member_ptr = atom_get<value_t>(argv);
                return 0;
            }
            static err_t setter(object_t *x, void *, long argc, t_atom *argv) requires (std::is_enum_v<value_t>) {
                if (argc != 1) {
                    object_error((t_object *) x, "Expected 1 argument, got %ld", argc);
                    return MAX_ERR_GENERIC;
                }
                auto value = magic_enum::enum_cast<value_t>(atom_getsym(argv)->s_name);
                if (!value) {
                    std::string error_string = "Invalid value: " + std::string(atom_getsym(argv)->s_name) + ". ";
                    error_string += "Must be one of: ";
                    for (auto &val : magic_enum::enum_names<value_t>()) {
                        error_string += "\"";
                        error_string += val;
                        error_string += "\" ";
                    }
                    object_error((t_object *) x, error_string.c_str());
                    return MAX_ERR_GENERIC;
                }
                x->*member_ptr = value.value_or(value_t{});
                return MAX_ERR_NONE;
            }
        };

        template <auto member_ptr>
        offset_attr_builder<member_ptr>::offset_attr_builder(t_class *c, std::string name)
            : attr_builder<offset_attr_builder>(c, name, type_to_symbol<value_t>(), member_pointer_info<member_ptr>::offset()) {

            if constexpr (std::is_enum_v<value_t>) {
                class_attr_addattr_parse(c, this->name.c_str(), "style", gensym("symbol"), 0, "enum");
                std::string enum_vals;
                for (auto val: magic_enum::enum_names<value_t>()) {
                    if (!enum_vals.empty()) {
                        enum_vals += " ";
                    }
                    enum_vals += val;
                }
                class_attr_addattr_parse(c, this->name.c_str(), "enumvals", gensym("symbol"), 0, enum_vals.c_str());
                object_method(this->attr, gensym("setmethod"), gensym("set"), (method) &setter);
                object_method(this->attr, gensym("setmethod"), gensym("get"), (method) &getter);
            }

            if constexpr (std::is_same_v<value_t, bool>) {
                class_attr_addattr_parse(c, this->name.c_str(), "style", gensym("symbol"), 0, "onoff");
            }

            if constexpr (std::is_arithmetic_v<value_t>) {
                class_attr_addattr_parse(c, this->name.c_str(), "style", gensym("symbol"), 0, "number");
            }
        }

        template <typename Object, typename Value>
        class method_attr_builder : public attr_builder<method_attr_builder<Object, Value>> {
        public:
            template <typename Getter, typename Setter>
            requires GetterFn<Getter, Object, Value> && SetterFn<Setter, Object, Value>
            method_attr_builder(t_class *c, std::string name, Getter &&getter, Setter &&setter)
                : attr_builder<method_attr_builder>(c, name, type_to_symbol<Value>()) {
                static const auto s_getter = getter;
                static const auto s_setter = setter;

                const auto getter_method = [](Object *x, void *, long *argc, t_atom **argv) -> t_atom_long {
                    Value value = std::invoke(s_getter, x);
                    char alloc;
                    atom_alloc(argc, argv, &alloc);
                    atom_set(*argv, value);
                    return 0;
                };

                const auto setter_method = [](Object *x, void *, long argc, t_atom *argv) -> t_atom_long {
                    if (argc != 1) {
                        object_error((t_object *) x, "Expected 1 argument, got %ld", argc);
                        return MAX_ERR_GENERIC;
                    }
                    return std::invoke(s_setter, x, atom_get<Value>(argv));
                };

                object_method(this->attr, gensym("setmethod"), gensym("get"), (method) +getter_method);
                object_method(this->attr, gensym("setmethod"), gensym("set"), (method) +setter_method);
            }
        };
    }

    template <auto member_ptr>
    auto create_attr(t_class *c, std::string name = std::string{detail::get_name<member_ptr>()}) {
        return detail::offset_attr_builder<member_ptr>(c, name);
    }

    template <typename Getter, typename Setter>
    auto create_attr(t_class *c, std::string name, Getter &&getter, Setter &&setter) {
        using getter_object_t = std::remove_pointer_t<detail::nth_arg_type_t<Getter, 0>>;
        using setter_object_t = std::remove_pointer_t<detail::nth_arg_type_t<Setter, 0>>;
        static_assert(std::is_same_v<getter_object_t, setter_object_t>, "Getter and setter must have the same object type");

        using getter_value_t = detail::return_type_t<Getter>;
        using setter_value_t = detail::nth_arg_type_t<Setter, 1>;
        static_assert(std::is_same_v<getter_value_t, setter_value_t>, "Getter and setter must have the same value type");

        using object_t = getter_object_t;
        using value_t = getter_value_t;

        return detail::method_attr_builder<object_t, value_t>(c, name, std::forward<Getter>(getter), std::forward<Setter>(setter));
    }
}

#endif //ATTRIBUTES_HPP
