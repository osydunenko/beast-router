#pragma once

#include <functional>
#include <vector>
#include <regex>
#include <tuple>

#include "../common/utility.hpp"

namespace beast_router {
namespace base { 
namespace clb {

/// Encapsulates and stores callbacks associated with resources 
template<
    class Session 
>
class storage
{
    struct callback; 

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
            utility::is_all_true_v<
                std::is_invocable_v<OnRequest, const request_type &, context_type &, const std::smatch &>...
            > && sizeof...(OnRequest) >= 1
        >
    >
    storage(OnRequest &&...on_request)
        : m_clbs{}
    {
        auto tuple = std::make_tuple(std::forward<OnRequest>(on_request)...);
        constexpr auto size = std::tuple_size<decltype(tuple)>::value;

        m_clbs.reserve(size);

        for (size_t idx = 0; idx < size; ++idx) {
            utility::tuple_func_idx(
                idx,
                tuple,
                std::make_index_sequence<size>{},
                [&](auto entry) -> bool {
                    m_clbs.push_back(
                        std::make_unique<callback_impl<decltype(entry)>>(entry)
                    );
                    return true;
                }
            );
        }
    }

    size_t size() const
    {
        return m_clbs.size();
    }

    void begin_execute(const request_type &request, context_type &&ctx, std::smatch &&match)
    {
        for (size_t idx = 0; idx < m_clbs.size(); ++idx) {
            if (!(*m_clbs[idx])(request, ctx, match)) {
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
        using return_type = utility::func_traits_result_t<Func>;

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

    container_type m_clbs;
};

} // namespace clb
} // namespace base
} // namespace beast_router
