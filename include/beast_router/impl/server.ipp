#pragma once

#include <thread>

#define SERVER_TEMPLATE_DECLARE \
    template <class Session, class Listener, class EventLoop>
#define SERVER_TEMPLATE_ATTRIBUTES \
    Session, Listener, EventLoop

namespace beast_router {

using namespace std::chrono_literals;

SERVER_TEMPLATE_DECLARE
server<SERVER_TEMPLATE_ATTRIBUTES>::server(endpoint_type endpoint)
    : m_endpoint { std::move(endpoint) }
    , m_event_loop { event_loop_type::create(std::thread::hardware_concurrency()) }
    , m_router {}
    , m_recv_timeout { 5s }
{
}

SERVER_TEMPLATE_DECLARE
server<SERVER_TEMPLATE_ATTRIBUTES>::server(threads_num_type thrs, endpoint_type endpoint)
    : server { endpoint }
{
    m_event_loop = event_loop_type::create(thrs);
}

SERVER_TEMPLATE_DECLARE
ROUTER_DECL int server<SERVER_TEMPLATE_ATTRIBUTES>::exec()
{
    BOOST_ASSERT(m_event_loop);

    auto on_accept = [this](typename listener_type::socket_type socket) {
        session_type::recv(std::move(socket), m_router, receive_timeout(),
            [](boost::system::error_code, std::string_view) {});
    };

    auto on_error = [this](boost::system::error_code ec, std::string_view) {
        if (ec == boost::system::errc::address_in_use or ec == boost::system::errc::permission_denied) {
            BOOST_ASSERT(m_event_loop);
            stop();
        }
    };

    listener_type::launch(*m_event_loop, m_endpoint, std::move(on_accept), std::move(on_error));

    return m_event_loop->exec();
}

SERVER_TEMPLATE_DECLARE
ROUTER_DECL void server<SERVER_TEMPLATE_ATTRIBUTES>::stop()
{
    BOOST_ASSERT(m_event_loop);
    m_event_loop->stop();
}

SERVER_TEMPLATE_DECLARE
template <class Tag, class... OnRequest>
ROUTER_DECL typename server<SERVER_TEMPLATE_ATTRIBUTES>::self_type&
server<SERVER_TEMPLATE_ATTRIBUTES>::on(const std::string& path, OnRequest&&... actions)
{
    if constexpr (std::is_same_v<Tag, get_t>) {
        m_router.get(path, std::forward<OnRequest>(actions)...);
    } else if constexpr (std::is_same_v<Tag, post_t>) {
        m_router.post(path, std::forward<OnRequest>(actions)...);
    } else if constexpr (std::is_same_v<Tag, put_t>) {
        m_router.put(path, std::forward<OnRequest>(actions)...);
    } else if constexpr (std::is_same_v<Tag, delete_t>) {
        m_router.delete_(path, std::forward<OnRequest>(actions)...);
    }
    return *this;
}

} // namespace beast_router