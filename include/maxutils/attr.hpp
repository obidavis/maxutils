//
// Created by Obi Davis on 02/05/2024.
//

#ifndef ATTR_HPP
#define ATTR_HPP

#include "ext.h"
#include "c74_jitter.h"
#include <string>
#include <sstream>
#include <type_traits>
#include <cctype>

#include "magic_enum.hpp"

#include "detail/attr_helpers.hpp"
#include "detail/reflection.hpp"
#include "detail/fixed_string.hpp"
#include "detail/member_pointer.hpp"
#include "detail/concepts.hpp"
#include "detail/functors.hpp"

namespace maxutils {
    using namespace c74::max;

    template <auto Member>
    static constexpr bool BooleanMember = std::is_same_v<detail::member_pointer_value_type_t<Member>, bool>;

    template <auto Member>
    static constexpr bool NumericMember =
        !BooleanMember<Member> && (
            std::is_integral_v<std::remove_all_extents_t<detail::member_pointer_value_type_t<Member>>> ||
            std::is_floating_point_v<std::remove_all_extents_t<detail::member_pointer_value_type_t<Member>>>
            );

    template <auto Member>
    static constexpr bool EnumMember = std::is_enum_v<detail::member_pointer_value_type_t<Member>>;

    static std::string get_default_label(const std::string &name) {
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

    template <typename T, typename ObjectT>
    concept HasGetter = requires(T t, ObjectT *x, void *attr, long *argc, t_atom **argv) {
        { T::getter(x, attr, argc, argv) } -> std::same_as<t_jit_err>;
    };

    template <typename T, typename ObjectT>
    concept HasSetter = requires(T t, ObjectT *x, void *attr, long argc, t_atom *argv) {
        { T::setter(x, attr, argc, argv) } -> std::same_as<t_jit_err>;
    };

    template <typename T, typename ObjectT>
    concept PureEffect = std::is_same_v<T, t_jit_err (*)(ObjectT *)> || requires(T t, ObjectT *x) {
        { +t(x) } -> std::same_as<t_jit_err>;
    };

    template <typename T, typename ObjectT>
    concept AttrBuilderChild = HasGetter<T, ObjectT> && HasSetter<T, ObjectT>;

    template <typename Derived, typename ObjectT, typename ValueT>
    class AttrBuilderBase {
        using object_t = ObjectT;
        using value_t = ValueT;
    public:
        AttrBuilderBase(t_class *c, std::string name, long offset = 0)
            : c{c}, name{std::move(name)}, offset{offset}, type{detail::type_to_symbol<value_t>()} {
            if constexpr (std::is_array_v<value_t>) {
                long size = std::extent_v<value_t>;
                attr = attr_offset_array_new(this->name.c_str(), type, size, 0, nullptr, nullptr, 0, offset);
            } else {
                attr = attr_offset_new(this->name.c_str(), type, 0, nullptr, nullptr, offset);
            }
            class_addattr(c, attr);
            with_label(get_default_label(this->name));
        }

        Derived &with_label(const std::string &label) {
            class_attr_addattr_format(c, name.c_str(), "label", gensym("symbol"), 0, "s", gensym_tr(label.c_str()));
            return static_cast<Derived &>(*this);
        }

        template <auto Effect>
        requires PureEffect<decltype(Effect), object_t>
        Derived &with_effect() {
            auto setter = [](object_t *x, void *attr, long argc, t_atom *argv) {
                Derived::setter(x, attr, argc, argv);
                Effect(x);
                return JIT_ERR_NONE;
            };
            object_method(attr, gensym("setmethod"), gensym("set"), (method) +setter);
            return static_cast<Derived &>(*this);
        }

        Derived &with_flags(long flags) {
            long old_flags = (long)object_method(attr, gensym("getflags"));
            object_method(attr, gensym("setflags"), old_flags | flags);
            return static_cast<Derived &>(*this);
        }

        Derived &get_defer_low_set_usurp_low() {
            return with_flags(JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_USURP_LOW);
        }
    protected:
        t_class *c;
        std::string name;
        long offset;
        t_symbol *type;
        t_object *attr;
    };

    template <auto Member>
    class OffsetAttrBuilder;

    template <auto Member>
    requires EnumMember<Member>
    class OffsetAttrBuilder<Member> : public AttrBuilderBase<
        OffsetAttrBuilder<Member>,
        detail::member_pointer_object_type_t<Member>,
        detail::member_pointer_value_type_t<Member>> {
    public:
        using object_t = detail::member_pointer_object_type_t<Member>;
        using value_t = detail::member_pointer_value_type_t<Member>;
        using Base = AttrBuilderBase<OffsetAttrBuilder, object_t, value_t>;
        OffsetAttrBuilder(t_class *c, std::string custom_name = std::string(detail::get_name<Member>()) )
            : Base(c, custom_name) {
            class_attr_addattr_parse(c, this->name.c_str(), "style", gensym("symbol"), 0, "enum");
            std::string enum_vals;
            for (auto val: magic_enum::enum_names<value_t>()) {
                if (!enum_vals.empty()) {
                    enum_vals += " ";
                }
                enum_vals += val;
            }
            class_attr_addattr_parse(c, this->name.c_str(), "enumvals", gensym("symbol"), 0, enum_vals.c_str());
            object_method(this->attr, gensym("setmethod"), gensym("set"), (method) setter);
            object_method(this->attr, gensym("setmethod"), gensym("get"), (method) getter);
        }

        static t_jit_err getter(object_t *x, void *, long *argc, t_atom **argv) {
            auto value = x->*Member;
            std::string value_str(magic_enum::enum_name(value));
            char alloc;
            atom_alloc(argc, argv, &alloc);
            atom_setsym(*argv, gensym(value_str.c_str()));
            return JIT_ERR_NONE;
        }

        static t_jit_err setter(object_t *x, void *, long argc, t_atom *argv) {
            if (argc != 1) {
                object_error((t_object *) x, "Expected 1 argument");
                return JIT_ERR_GENERIC;
            }
            auto value_str = atom_getsym(argv)->s_name;
            auto value = magic_enum::enum_cast<value_t>(value_str);
            if (!value.has_value()) {
                std::stringstream error_ss;
                error_ss << "Invalid value: \"" << value_str << "\". ";
                error_ss << "Must be one of: ";
                for (auto val: magic_enum::enum_names<value_t>()) {
                    error_ss << "\"" << val << "\" ";
                }

                object_error((t_object *) x, error_ss.str().c_str());
                return JIT_ERR_GENERIC;
            }
            x->*Member = value.value_or(value_t{}); // For compatibility with macOS < 10.13
            return JIT_ERR_NONE;
        }

        // OffsetAttrBuilder &style_with_enum_indices() {
        //     class_attr_addattr_parse(this->c, this->name.c_str(), "style", gensym("symbol"), 0, "enumindex");
        //     return *this;
        // }
    };

    template <auto Member>
    requires BooleanMember<Member>
    class OffsetAttrBuilder<Member> : public AttrBuilderBase<
        OffsetAttrBuilder<Member>,
        detail::member_pointer_object_type_t<Member>,
        detail::member_pointer_value_type_t<Member>> {
        using object_t = detail::member_pointer_object_type_t<Member>;
        using value_t = detail::member_pointer_value_type_t<Member>;
        using Base = AttrBuilderBase<OffsetAttrBuilder, object_t, value_t>;
    public:
        OffsetAttrBuilder(t_class *c, std::string custom_name = std::string(detail::get_name<Member>()) )
            : Base(c, custom_name, detail::member_pointer_info<Member>::offset()) {
            class_attr_addattr_parse(c, this->name.c_str(), "style", gensym("symbol"), 0, "onoff");
        }

        static auto getter(object_t *x, void *, long *argc, t_atom **argv) -> t_jit_err {
            auto value = x->*Member;
            char alloc;
            atom_alloc(argc, argv, &alloc);
            atom_setlong(*argv, value);
            return JIT_ERR_NONE;
        }

        static auto setter(object_t *x, void *, long argc, t_atom *argv) -> t_jit_err {
            if (argc != 1) {
                object_error((t_object *) x, "Expected 1 argument");
                return JIT_ERR_GENERIC;
            }
            auto value = atom_getlong(argv);
            x->*Member = value;
            return JIT_ERR_NONE;
        }
    };

    template <auto Member>
    requires NumericMember<Member>
    class OffsetAttrBuilder<Member> : public AttrBuilderBase<
        OffsetAttrBuilder<Member>,
        detail::member_pointer_object_type_t<Member>,
        detail::member_pointer_value_type_t<Member>> {
        using object_t = detail::member_pointer_object_type_t<Member>;
        using value_t = detail::member_pointer_value_type_t<Member>;
        using Base = AttrBuilderBase<OffsetAttrBuilder, object_t, value_t>;
    public:
        OffsetAttrBuilder(t_class *c, std::string custom_name = std::string(detail::get_name<Member>()) )
            : Base(c, custom_name, detail::member_pointer_info<Member>::offset()) {
            class_attr_addattr_parse(c, this->name.c_str(), "style", gensym("symbol"), 0, "number");
        }

        static auto getter(object_t *x, void *, long *argc, t_atom **argv) -> t_jit_err {
            auto value = x->*Member;
            char alloc;
            atom_alloc(argc, argv, &alloc);
            atom_set(*argv, value);
            return JIT_ERR_NONE;
        }

        static auto setter(object_t *x, void *, long argc, t_atom *argv) -> t_jit_err {
            if (argc != 1) {
                object_error((t_object *) x, "Expected 1 argument");
                return JIT_ERR_GENERIC;
            }
            x->*Member = atom_get<value_t>(argv);
            return JIT_ERR_NONE;
        }

        OffsetAttrBuilder &with_min_max(double min, double max) {
            attr_addfilter_clip(this->attr, min, max, 1, 1);
            return *this;
        }

        template <auto Pred, detail::fixed_string Error>
        requires PurePredicate<decltype(Pred), value_t>
        OffsetAttrBuilder &with_predicate() {
            auto setter = [](object_t *x, void *, long argc, t_atom *argv) {
                value_t v = atom_get<value_t>(argv);
                if (!Pred(v)) {
                    object_error((t_object *) x, Error);
                    return JIT_ERR_GENERIC;
                }
                x->*Member = v;
                return JIT_ERR_NONE;
            };
            object_method(this->attr, gensym("setmethod"), gensym("set"), (method) +setter);
            return *this;
        }
    };

    template <typename T, typename ObjectT, typename ValueT>
    concept AttrGetter = requires(T t, ObjectT *x) {
        { t(x) } -> std::same_as<ValueT>;
    };

    template <typename T, typename ObjectT, typename ValueT>
    concept AttrSetter = requires(T t, ObjectT *x, ValueT value) {
        { t(x, value) } -> std::same_as<t_jit_err>;
    };



    template <typename Getter, typename Setter>
    class MethodAttrBuilder : public AttrBuilderBase<
        MethodAttrBuilder<Getter, Setter>,
        std::remove_pointer_t<detail::nth_arg_type_t<Getter, 0>>,
        detail::return_type_t<Getter>> {
        using object_t = std::remove_pointer_t<detail::nth_arg_type_t<Getter, 0>>;
        using value_t = detail::return_type_t<Getter>;
        using Base = AttrBuilderBase<MethodAttrBuilder, object_t, value_t>;
    public:
        MethodAttrBuilder(t_class *c, std::string custom_name, Getter getter, Setter setter)
            : Base(c, custom_name) {
            object_method(this->attr, gensym("setmethod"), gensym("set"), (method) MethodAttrBuilder::setter);
            object_method(this->attr, gensym("setmethod"), gensym("get"), (method) MethodAttrBuilder::getter);
            if constexpr (std::is_same_v<value_t, bool>) {
                class_attr_addattr_parse(c, this->name.c_str(), "style", gensym("symbol"), 0, "onoff");
            }
        }

        static t_jit_err getter(object_t *x, void *, long *argc, t_atom **argv) {
            auto value = Getter{}(x);
            char alloc;
            atom_alloc(argc, argv, &alloc);
            atom_set(*argv, value);
            return JIT_ERR_NONE;
        }

        static t_jit_err setter(object_t *x, void *, long argc, t_atom *argv) {
            if (argc != 1) {
                object_error((t_object *) x, "Expected 1 argument");
                return JIT_ERR_GENERIC;
            }
            Setter{}(x, atom_get<value_t>(argv));
            return JIT_ERR_NONE;
        }
    };

    // template<auto Member>
    // auto create_attr(t_class *c, std::string name = std::string(detail::get_name<Member>())) {
    //     return OffsetAttrBuilder<Member>(c, name);
    // }
    //
    // template <typename Getter, typename Setter>
    // auto create_attr(t_class *c, std::string name, Getter getter, Setter setter) {
    //     return MethodAttrBuilder<Getter, Setter>(c, name, getter, setter);
    // }


    template <auto Member>
    struct attr_base {
        attr_base(int x) {}
    };

    constexpr auto attr = [](auto &&m) constexpr {
        return attr_base<m>(0);
    };
}

#endif //ATTR_HPP
