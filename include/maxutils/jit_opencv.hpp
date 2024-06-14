//
// Created by Obi Davis on 14/06/2024.
//

#ifndef JIT_OPENCV_HPP
#define JIT_OPENCV_HPP

#include "jit.common.h"
#include <algorithm>
#include "opencv2/core/mat.hpp"

namespace maxutils {
    inline cv::Mat jit_matrix_to_cv_mat(void *matrix) {
        using namespace c74::max;

        if (!matrix) {
            char error[] = "Error converting Jitter matrix: invalid pointer.";
            jit_object_error((t_object *) matrix, error);
            return {};
        }

        t_jit_matrix_info info;
        auto err = (t_jit_err) jit_object_method(matrix, _jit_sym_getinfo, &info);
        if (err) {
            char error[] = "Error converting Jitter matrix: invalid info.";
            jit_object_error((t_object *) matrix, error);
            return {};
        }

        // if (info.dimcount != 2) {
        //     char error[] = "Error converting Jitter matrix: invalid dimension count.";
        //     jit_object_error((t_object *) matrix, error);
        //     return {};
        // }

        const auto type = [](t_symbol *type, long planecount) -> int {
            if (type == _jit_sym_char) {
                return CV_8UC(planecount);
            }
            if (type == _jit_sym_long) {
                return CV_32SC(planecount);
            }
            if (type == _jit_sym_float32) {
                return CV_32FC(planecount);
            }
            if (type == _jit_sym_float64) {
                return CV_64FC(planecount);
            }
            return -1;
        }(info.type, info.planecount);

        void *data;
        err = (t_jit_err) jit_object_method(matrix, _jit_sym_getdata, &data);
        if (err) {
            char error[] = "Error converting Jitter matrix: invalid data.";
            jit_object_error((t_object *) matrix, error);
            return {};
        }

        int dims = info.dimcount;
        int sizes[JIT_MATRIX_MAX_DIMCOUNT]{};
        std::copy_n(info.dim, info.dimcount, sizes);
        std::swap(sizes[0], sizes[1]);
        size_t steps[JIT_MATRIX_MAX_DIMCOUNT]{};
        std::copy_n(info.dimstride, info.dimcount, steps);
        std::swap(steps[0], steps[1]);
        return {
            dims,
            sizes,
            type,
            data,
            steps
        };
    }
}
#endif //JIT_OPENCV_HPP
