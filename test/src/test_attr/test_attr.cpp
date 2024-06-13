//
// Created by Obi Davis on 12/06/2024.
//

#include "ext.h"
#include "maxutils/attr.hpp"

using namespace c74::max;

struct t_test_attr {
    t_object ob;
    bool boolean_attr;
    char char_attr;
    long long_attr;
    float float_attr;
    double double_attr;
    enum class Enum { A, B, C } enum_attr;
    t_symbol *symbol_attr;
    t_outlet *outlet;
};

static t_class *c;

BEGIN_USING_C_LINKAGE

t_test_attr *test_attr_new(t_symbol *s, long argc, t_atom *argv);
void test_attr_free(t_test_attr *x);

void ext_main(void *) {
    c = class_new("test_attr", (method)test_attr_new, (method)test_attr_free, sizeof(t_test_attr), nullptr, A_GIMME, 0);
    class_register(CLASS_BOX, c);

    maxutils::create_attr<&t_test_attr::boolean_attr>(c);
    maxutils::create_attr<&t_test_attr::char_attr>(c);
    maxutils::create_attr<&t_test_attr::long_attr>(c);
    maxutils::create_attr<&t_test_attr::float_attr>(c);
    maxutils::create_attr<&t_test_attr::double_attr>(c);
    maxutils::create_attr<&t_test_attr::enum_attr>(c);
    // maxutils::create_attr<&t_test_attr::symbol_attr>(c);
}

END_USING_C_LINKAGE

t_test_attr *test_attr_new(t_symbol *s, long argc, t_atom *argv) {
    auto *x = (t_test_attr *)object_alloc(c);
    x->outlet = outlet_new(x, nullptr);
    return x;
}

void test_attr_free(t_test_attr *x) {
    outlet_delete(x->outlet);
}