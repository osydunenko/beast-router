#pragma once

#include <chrono>
#include <tuple>
#include <type_traits>

namespace beast_router {
namespace utility {
    namespace details {

        template <class T>
        struct is_chrono_duration : std::false_type { };

        template <class Rep, class Period>
        struct is_chrono_duration<std::chrono::duration<Rep, Period>> : std::true_type {
        };

        template <bool...>
        struct bool_pack;
        template <bool... V>
        using is_all_true = std::is_same<bool_pack<true, V...>, bool_pack<V..., true>>;

        template <class T, class = void>
        struct func_traits_result_type {
            using result_type = void;
        };

        template <class T>
        struct func_traits_result_type<T, std::void_t<typename T::result_type>> {
            using result_type = typename T::result_type;
        };

        template <class R, class... Args>
        struct func_traits_result_type<R (*)(Args...)> {
            using result_type = R;
        };

        template <class R, class... Args>
        struct func_traits_result_type<R (&)(Args...)> {
            using result_type = R;
        };

        template <class Class, class... Args>
        struct is_class_creatable {
            static_assert(std::is_class_v<Class>, "Class requirements are not met");

            using one = char;

            template <class C, class... A>
            static auto test(int) -> decltype(C { std::declval<A>()... }, one());

            template <class C, class... A>
            static char (&test(...))[2];

            enum { value = sizeof(test<Class, Args...>(0) == sizeof(one)) };
        };

    } // namespace details

    /// Type Trait for testing chrono duration
    template <class T>
    constexpr bool is_chrono_duration_v = details::is_chrono_duration<T>::value;

    /// Is All are True parameter types checker
    template <bool... V>
    constexpr bool is_all_true_v = details::is_all_true<V...>::value;

    /// Type Trait for testing class creation
    template <class Class, class... Args>
    constexpr bool is_class_creatable_v = details::is_class_creatable<Class, Args...>::value;

    /// Type Trait for unpack a function return value
    template <class T>
    using func_traits_result_t =
        typename details::func_traits_result_type<T>::result_type;

    /// Iterator method over a tuple
    /*
     * Iterates over a tuple and invokes respective func by index
     *
     * @param idx index for execution
     * @param tuple Tuple within the data
     * @param func Refrence to the callback
     * @return void
     */
    template <class Tuple, class Func, std::size_t... Idxs>
    void tuple_func_idx(std::size_t idx, const Tuple& tuple,
        std::index_sequence<Idxs...>, Func&& func)
    {
        [](...) {
        }((idx == Idxs && std::forward<Func>(func)(std::get<Idxs>(tuple)))...);
    }

} // namespace utility
} // namespace beast_router
