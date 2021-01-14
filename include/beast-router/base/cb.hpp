#pragma once

#include <functional>
#include <type_traits>
#include <vector>
#include <regex>
#include <tuple>

namespace server {
namespace base { 
namespace cb {
namespace detail {

template<bool...> struct bool_pack;
template<bool...v>
using all_true = std::is_same<bool_pack<true, v...>, bool_pack<v..., true>>;

template<class Tuple, class Func, std::size_t ...Idxs>
void tuple_func_idx(std::size_t idx, const Tuple &tuple, std::index_sequence<Idxs...>, Func &&func)
{
    [](...){}(
        (idx == Idxs && std::forward<Func>(func)(std::get<Idxs>(tuple)))...
    );
}

template<class T>
struct func_traits
{
    using decayed_type = std::decay_t<T>;
    using return_type = typename func_traits<decltype(&decayed_type::operator())>::return_type;
};

template<class C, class R, class ...Args>
struct func_traits<R(C::*)(Args...) const>
{
    using return_type = R;
};

template<class C, class R, class ...Args>
struct func_traits<R(C::*)(Args...)>
{
    using return_type = R;
};

template<class R, class ...Args>
struct func_traits<R(*)(Args...)>
{
    using return_type = R;
};

} // namespace detail

template<
    class Session 
>
class storage
{
    struct callback; 

    template<class Func>
    struct callback_impl;
public:
    using self_type = storage<Session>;

    using session_type = Session;

    using session_context_type = typename session_type::context_type;

    using request_type = typename session_type::request_type;

    using context_type = typename session_type::context_type;

    using container_type = std::vector<std::unique_ptr<callback>>;

    storage() = delete;

    template<
        class ...OnRequest,
        typename = std::enable_if_t<
            detail::all_true<
                std::is_invocable_v<OnRequest, const request_type &, context_type &, const std::smatch &>...
            >::value && sizeof...(OnRequest) >= 1
        >
    >
    storage(OnRequest &&...on_request)
        : m_cbs{}
    {
        auto tuple = std::make_tuple(std::forward<OnRequest>(on_request)...);
        constexpr auto size = std::tuple_size<decltype(tuple)>::value;

        m_cbs.reserve(size);

        for (size_t idx = 0; idx < size; ++idx) {
            detail::tuple_func_idx(
                idx,
                tuple,
                std::make_index_sequence<size>{},
                [&](auto entry) -> bool {
                    m_cbs.push_back(
                        std::make_unique<callback_impl<decltype(entry)>>(entry)
                    );
                    return true;
                }
            );
        }
    }

    size_t size() const
    {
        return m_cbs.size();
    }

    void begin_execute(const request_type &request, context_type &&ctx, std::smatch &&match)
    {
        for (size_t idx = 0; idx < m_cbs.size(); ++idx) {
            if (!(*m_cbs[idx])(request, ctx, match)) {
                break;
            }
        }
    }

private:
    struct callback
    {
        virtual ~callback() = default;
        virtual bool operator()(const request_type &, context_type &, const std::smatch &) = 0;
    };

    template<class Func>
    struct callback_impl: callback
    {
        using return_type = typename detail::func_traits<Func>::return_type;

        callback_impl(Func func)
        {
            m_func = std::bind<return_type>(
                func,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3
            );
        }

        bool operator()(const request_type &req, context_type &ctx, const std::smatch &match) override
        {
            if constexpr (std::is_same<return_type, bool>::value) {
                return m_func(req, ctx, match);
            } else {
                m_func(req, ctx, match);
                return true;
            }
        }

        std::function<return_type(const request_type &, context_type &, const std::smatch &)> m_func;
    };

    container_type m_cbs;
};

} // namespace cb
} // namespace base
} // namespace server
