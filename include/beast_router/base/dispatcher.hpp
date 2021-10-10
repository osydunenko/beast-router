#pragma once

#include <type_traits>
#include <regex>
#include <string_view>

#include <boost/beast/http/message.hpp>

#include "config.hpp"
#include "lockable.hpp"

#include "../common/utility.hpp"

namespace beast_router {
namespace base {

template<class Session>
class dispatcher
{
public:
    using session_type = Session;

    using router_type = typename session_type::router_type;

    using method_const_map_pointer = typename router_type::method_const_map_pointer;

    using resource_map_type = typename router_type::resource_map_type;

    using method_type = typename router_type::method_type;

    using mutex_pointer_type = typename router_type::mutex_pointer_type;

    using impl_type = typename session_type::impl_type;

    using context_type = typename session_type::context_type;

    using storage_type = typename router_type::storage_type;

    explicit dispatcher(const router_type &router)
        : m_method_map{router.get_resource_map()}
        , m_mutex{router.get_mutex_pointer()}
    {
    }

    template<
        bool IsMessageRequest,
        class MessageBody,
        class Fields
    >
    typename std::enable_if_t<IsMessageRequest>
    do_process_request(boost::beast::http::message<IsMessageRequest, MessageBody, Fields> &&request, impl_type &impl)
    {
        static_assert(utility::is_all_true_v<
            session_type::is_request::value == IsMessageRequest,
            std::is_same_v<MessageBody, typename session_type::body_type>
        >, "dispatcher::do_process_request requirements are not met");

        assert(m_method_map);

        LOCKABLE_ENTER_TO_READ(m_mutex);

        std::string_view target = request.target();
        method_type method = request.method();
        bool is_handled = false;

        // Handle a method by checking if a handler was assigned in the resource map
        if (const auto method_pos = m_method_map->find(method); method_pos != m_method_map->cend()) {
            auto &resource_map = method_pos->second;
            std::for_each(
                resource_map.begin(),
                resource_map.end(),
                [&](auto &val) {
                    const std::regex re{val.first};
                    const std::string target_string{target};
                    std::smatch base_match;
                    if (std::regex_match(target_string, base_match, re)) {
                        if (const_cast<storage_type &>(val.second)
                                .begin_execute(request, context_type{impl}, std::move(base_match))) {
                            is_handled = true;
                        }
                    }
                }
            );
        }

        // Handle the "not_found" case which is the unknown method
        if (const auto not_found = m_method_map->find(method_type::unknown); !is_handled) {
            ROUTER_ASSUME((not_found != m_method_map->cend()));
            auto &resource_map = not_found->second;
            if (const auto storage = resource_map.find("");
                storage != resource_map.cend()) {
                const_cast<storage_type &>(storage->second)
                    .begin_execute(request, context_type{impl}, {});
            }
        }
    }

    template<
        bool IsMessageRequest,
        class MessageBody,
        class Fields
    >
    typename std::enable_if_t<!IsMessageRequest>
    do_process_request(boost::beast::http::message<IsMessageRequest, MessageBody, Fields> &&request, impl_type &impl)
    {
        static_assert(utility::is_all_true_v<
            session_type::is_request::value == IsMessageRequest,
            std::is_same_v<MessageBody, typename session_type::body_type>
        >, "dispatcher::do_process_request requirements are not met");

        assert(m_method_map);

        LOCKABLE_ENTER_TO_READ(m_mutex);

        const auto not_found = m_method_map->find(method_type::unknown);
        ROUTER_ASSUME((not_found != m_method_map->cend()));
        auto &resource_map = not_found->second;
        if (const auto storage = resource_map.find("");
            storage != resource_map.cend()) {
            const_cast<storage_type &>(storage->second)
                .begin_execute(request, context_type{impl}, {});
        }
    }

private:
    method_const_map_pointer m_method_map;
    mutex_pointer_type m_mutex;
};

} // namespace base
} // namespace beast_router
