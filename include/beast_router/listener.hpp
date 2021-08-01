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

namespace beast_router {

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

} // namespace beast_router

#include "impl/listener.hpp"

