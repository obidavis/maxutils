//
// Created by Obi Davis on 13/06/2024.
//

#ifndef T_OBJECT_RAII_HPP
#define T_OBJECT_RAII_HPP

#include <memory>

#include "ext.h"

namespace maxutils {
    using t_object_ptr = std::unique_ptr<t_object, decltype([](t_object *p) { object_free(p); })>;
}

#endif //T_OBJECT_RAII_HPP
