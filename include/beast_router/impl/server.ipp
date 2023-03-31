#pragma once

#include "../common/event_loop.hpp"

#define SERVER_TEMPLATE_DECLARE \
    template <class Session>
#define SERVER_TEMPLATE_ATTRIBUTES \
    Session

namespace beast_router {

SERVER_TEMPLATE_DECLARE
server<SERVER_TEMPLATE_ATTRIBUTES>::server()
    : m_router {}
    , m_address { boost::asio::ip::address_v4::any() }
    , m_port { 8080 }
    , m_recv_timeout { 5s }
{
}

SERVER_TEMPLATE_DECLARE
ROUTER_DECL int server<SERVER_TEMPLATE_ATTRIBUTES>::exec()
{
    auto on_accept = [this](http_listener::socket_type socket) {
        session_type::recv(std::move(socket), m_router, recv_timeout(),
            [](boost::system::error_code, std::string_view) {});
    };

    auto on_error = [this](boost::system::error_code ec, std::string_view) {
        if (ec == boost::system::errc::address_in_use or ec == boost::system::errc::permission_denied) {
            EVENT_IOC.stop();
        }
    };

    http_listener::launch(EVENT_IOC, { m_address, m_port }, std::move(on_accept), std::move(on_error));

    return EVENT_LOOP.exec();
}

SERVER_TEMPLATE_DECLARE
ROUTER_DECL void server<SERVER_TEMPLATE_ATTRIBUTES>::set_address(const std::string& address)
{
    m_address = address_type::from_string(address);
}

SERVER_TEMPLATE_DECLARE
ROUTER_DECL std::string server<SERVER_TEMPLATE_ATTRIBUTES>::address() const
{
    return m_address.to_string();
}

SERVER_TEMPLATE_DECLARE
ROUTER_DECL void server<SERVER_TEMPLATE_ATTRIBUTES>::set_port(port_type port)
{
    m_port = port;
}

SERVER_TEMPLATE_DECLARE
ROUTER_DECL typename server<SERVER_TEMPLATE_ATTRIBUTES>::port_type
server<SERVER_TEMPLATE_ATTRIBUTES>::port() const
{
    return m_port;
}

SERVER_TEMPLATE_DECLARE
ROUTER_DECL void server<SERVER_TEMPLATE_ATTRIBUTES>::set_recv_timeout(duration_type timeout)
{
    m_recv_timeout = timeout;
}

SERVER_TEMPLATE_DECLARE
ROUTER_DECL typename server<SERVER_TEMPLATE_ATTRIBUTES>::duration_type
server<SERVER_TEMPLATE_ATTRIBUTES>::recv_timeout() const
{
    return m_recv_timeout;
}

SERVER_TEMPLATE_DECLARE
template <class... OnRequest>
ROUTER_DECL typename server<SERVER_TEMPLATE_ATTRIBUTES>::self_type&
server<SERVER_TEMPLATE_ATTRIBUTES>::on_get(const std::string& path, OnRequest&&... actions)
{
    m_router.get(path, std::forward<OnRequest>(actions)...);
    return *this;
}

SERVER_TEMPLATE_DECLARE
template <class... OnRequest>
ROUTER_DECL typename server<SERVER_TEMPLATE_ATTRIBUTES>::self_type&
server<SERVER_TEMPLATE_ATTRIBUTES>::on_put(const std::string& path, OnRequest&&... actions)
{
    m_router.put(path, std::forward<OnRequest>(actions)...);
    return *this;
}

SERVER_TEMPLATE_DECLARE
template <class... OnRequest>
ROUTER_DECL typename server<SERVER_TEMPLATE_ATTRIBUTES>::self_type&
server<SERVER_TEMPLATE_ATTRIBUTES>::on_post(const std::string& path, OnRequest&&... actions)
{
    m_router.post(path, std::forward<OnRequest>(actions)...);
    return *this;
}

SERVER_TEMPLATE_DECLARE
template <class... OnRequest>
ROUTER_DECL typename server<SERVER_TEMPLATE_ATTRIBUTES>::self_type&
server<SERVER_TEMPLATE_ATTRIBUTES>::on_delete(const std::string& path, OnRequest&&... actions)
{
    m_router.delete_(path, std::forward<OnRequest>(actions)...);
    return *this;
}

} // namespace beast_router