#pragma once

#define LISTENER_TEMPLATE_DECLARE         \
    template<                             \
    class Protocol,                       \
    class Acceptor,                       \
    class Socket,                         \
    template<typename> class Endpoint>

#define CHECK_EC(ec, msg)         \
    if (ec) {                     \
        if (m_on_error) {         \
            m_on_error(ec, msg);  \
        }                         \
        return;                   \
    }

namespace beast_router {

LISTENER_TEMPLATE_DECLARE
listener<LISTENER_TEMPLATE_ATTRIBUTES>::listener(boost::asio::io_context &ctx, on_accept_type &&on_accept)
    : base::strand_stream{ctx.get_executor()} 
    , m_acceptor{ctx}
    , m_io_ctx{ctx}
    , m_on_accept{std::move(on_accept)}
    , m_on_error{nullptr}
{
}

LISTENER_TEMPLATE_DECLARE
listener<LISTENER_TEMPLATE_ATTRIBUTES>::listener(boost::asio::io_context &ctx, on_accept_type &&on_accept, on_error_type &&on_error)
    : base::strand_stream{ctx.get_executor()}
    , m_acceptor{ctx}
    , m_io_ctx{ctx}
    , m_on_accept{std::move(on_accept)}
    , m_on_error{std::move(on_error)}
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
    assert(m_on_accept != nullptr);
    CHECK_EC(ec, "accept/loop");
    m_on_accept(std::move(socket));
}

} // namespace beast_router
