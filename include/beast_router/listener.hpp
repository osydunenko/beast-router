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

/// Listens on the incoming connections.
/**
 * The common class which listens and accepts the incoming connections.
 *
 * ### Example
 * ```cpp
 * http_listener::on_error_type on_error = [](boost::system::error_code ec, boost::string_view v) {
 *      if (ec == boost::system::errc::address_in_use ||
 *          ec == boost::system::errc::permission_denied)
 *          g_ioc.stop();
 *  };

 *  http_listener::on_accept_type on_accept = [&on_error](http_listener::socket_type socket) {
 *      http_session::recv(std::move(socket), g_router, 5s, on_error);
 *  };

 *  const auto address = boost::asio::ip::address_v4::any();
 *  const auto port = static_cast<unsigned short>(8081);

 *  http_listener::launch(g_ioc, {address, port}, on_accept, on_error);
 * ```
 */
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
    /// Typedef referencing to self
    using self_type = listener<LISTENER_TEMPLATE_ATTRIBUTES>;

    /// Typedef referencing to the given Protocol
    using protocol_type = Protocol;

    /// Typedef referencing to the basic socket acceptor
    using acceptor_type = Acceptor;

    /// Typedef referencing to a stream socket
    using socket_type = Socket;

    /// Typedef referencing to the basic endpoint type
    using endpoint_type = Endpoint<protocol_type>;

    /// Typedef callback on accept
    using on_accept_type = std::function<void(socket_type)>;

    /// Typedef callback on error
    using on_error_type = std::function<void(boost::system::error_code, boost::string_view)>;

    /// Constructor
    /**
     * @param ctx A reference to the `boost::asio::io_context`
     * @param on_accept An rvalue reference on the callback on which invokes on new connection
     */
    explicit listener(boost::asio::io_context &ctx, on_accept_type &&on_accept);

    /// Constructor
    /**
     * @param ctx A reference to the `boost::asio::io_context`
     * @param on_accept An rvalue reference on the callback; invokes on new connection
     * @param on_error An rvalue reference on the callback; invokes on error 
     */
    explicit listener(boost::asio::io_context &ctx, on_accept_type &&on_accept, on_error_type &&on_error);

    /// The factory method to start listening
    /**
     * Create an instance of `self` and start listening by calling loop() method
     *
     * @param ctx A reference to the `boost::asio::io_context`
     * @param endpoint A const reference to the endpoint_type
     * @param on_action A list of actions suitable for the self construction i.e. `on_accept`, `on_error` signatures
     * @returns void
     */
    template<class ...OnAction>
    static auto launch(boost::asio::io_context &ctx, const endpoint_type &endpoint, OnAction &&...on_action)
        -> decltype(self_type(std::declval<boost::asio::io_context&>(), std::declval<OnAction>()...), void());

protected:
    /// Starts a loop on the given endpoint
    /**
     * @param Endpoint
     * @returns void
     */
    void loop(const endpoint_type &endpoint);

    /// An async accept method. Passes on_accept() as an internal callback which triggers on new connection
    void do_accept();

    /// An internal on_accept callback and passes the event to on_spawn_method through the loop
    void on_accept(boost::system::error_code ec, socket_type socket);

    /// Calls the given user specified on_accept callback
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

