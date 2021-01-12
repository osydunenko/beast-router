#pragma once

#include <memory>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/system/error_code.hpp>
#include <boost/utility/string_view.hpp>

#include "base/strand_stream.hpp"

#define LISTENER_TEMPLATE_ATTRIBUTES \
    Protocol, Acceptor, Socket, Endpoint

#define LISTENER_TEMPLATE_DECLARE         \
    template<                             \
    class Protocol,                       \
    class Acceptor,                       \
    class Socket,                         \
    template<typename> class Endpoint >

#define CHECK_EC(ec, msg)         \
    if (ec) {                     \
        if (m_on_error) {         \
            m_on_error(ec, msg);  \
        }                         \
        return;                   \
    }

namespace server {

template<
    class Protocol = boost::asio::ip::tcp,
    class Acceptor = boost::asio::basic_socket_acceptor<Protocol>,
    class Socket = boost::asio::basic_stream_socket<Protocol>,
    template<typename> class Endpoint = boost::asio::ip::basic_endpoint
> 
class listener: public base::strand_stream
    , public std::enable_shared_from_this<listener<LISTENER_TEMPLATE_ATTRIBUTES>>
{
public:
    using self_type = listener<LISTENER_TEMPLATE_ATTRIBUTES>;

    using protocol_type = Protocol;

    using acceptor_type = Acceptor;

    using socket_type = Socket;

    using endpoint_type = Endpoint<protocol_type>;

    using on_accept_type = std::function<void(socket_type)>;

    using on_error_type = std::function<void(boost::system::error_code, boost::string_view)>;

    explicit listener(boost::asio::io_context &ctx, const on_accept_type &on_accept);
    explicit listener(boost::asio::io_context &ctx, const on_accept_type &on_accept, const on_error_type &on_error);

    template<class ...OnAction>
    static auto launch(boost::asio::io_context &ctx, const endpoint_type &endpoint, const OnAction &...on_action)
        -> decltype(self_type(std::declval<boost::asio::io_context&>(), std::declval<OnAction>()...), void());

protected:
    void loop(const endpoint_type &endpoint);
    void do_accept();
    void on_accept(boost::system::error_code ec, socket_type socket);
    void on_spawn_connect(boost::system::error_code ec, socket_type &socket);

private:
    acceptor_type m_acceptor;
    boost::asio::io_context &m_io_ctx;
    on_accept_type m_on_accept;
    on_error_type m_on_error;
    endpoint_type m_endpoint;
};

using default_listener = listener<>;

LISTENER_TEMPLATE_DECLARE
template<class ...OnAction>
auto listener<LISTENER_TEMPLATE_ATTRIBUTES>::launch(boost::asio::io_context &ctx, const endpoint_type &endpoint, const OnAction &...on_action)
    -> decltype(self_type(std::declval<boost::asio::io_context&>(), std::declval<OnAction>()...), void())
{
    std::make_shared<self_type>(ctx, on_action...)->loop(endpoint);
}

LISTENER_TEMPLATE_DECLARE
listener<LISTENER_TEMPLATE_ATTRIBUTES>::listener(boost::asio::io_context &ctx, const on_accept_type &on_accept)
    : base::strand_stream(ctx.get_executor()) 
    , m_acceptor(ctx)
    , m_io_ctx(ctx)
    , m_on_accept(on_accept)
    , m_on_error(nullptr)
{
}

LISTENER_TEMPLATE_DECLARE
listener<LISTENER_TEMPLATE_ATTRIBUTES>::listener(boost::asio::io_context &ctx, const on_accept_type &on_accept, const on_error_type &on_error)
    : base::strand_stream(ctx.get_executor())
    , m_acceptor(ctx)
    , m_io_ctx(ctx)
    , m_on_accept(on_accept)
    , m_on_error(on_error)
{
}

LISTENER_TEMPLATE_DECLARE
void listener<LISTENER_TEMPLATE_ATTRIBUTES>::loop(const endpoint_type &endpoint)
{
    auto ec = boost::system::error_code{};

    m_acceptor.open(endpoint.protocol(), ec);
    CHECK_EC(ec, "open/loop");

    m_acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
    CHECK_EC(ec, "set_option/loop");

    m_acceptor.bind(endpoint, ec);
    CHECK_EC(ec, "bind/loop");

    m_acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
    CHECK_EC(ec, "listen/loop");

    m_endpoint = endpoint;

    do_accept();
}

LISTENER_TEMPLATE_DECLARE
void listener<LISTENER_TEMPLATE_ATTRIBUTES>::do_accept()
{
    m_acceptor.async_accept(m_io_ctx, std::bind(&self_type::on_accept, 
        this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

LISTENER_TEMPLATE_DECLARE
void listener<LISTENER_TEMPLATE_ATTRIBUTES>::on_accept(boost::system::error_code ec, socket_type socket)
{
    boost::asio::post(static_cast<base::strand_stream&>(*this),
        std::bind(&self_type::on_spawn_connect, this->shared_from_this(), ec, std::move(socket)));
    do_accept();
}

LISTENER_TEMPLATE_DECLARE
void listener<LISTENER_TEMPLATE_ATTRIBUTES>::on_spawn_connect(boost::system::error_code ec, socket_type &socket)
{
    CHECK_EC(ec, "accept/loop");
    m_on_accept(std::move(socket));
}

} // namespace server

