//
// Created by Obi Davis on 12/06/2024.
//

#include <type_traits>
#include <tuple>

int free_function_no_args();
int free_function_one_arg(double a);
int free_function_two_args(double a, long b);

template <typename T>
struct function_info;

template <typename Ret, typename ...Args>
struct function_info<Ret(*)(Args...)> {
    using return_type = Ret;
    using arg_types = std::tuple<Args...>;
};

template <typename T>
concept NonCapturingLambda = requires(T t) {
    { +t() } -> std::same_as<void>;
};

int main() {
}