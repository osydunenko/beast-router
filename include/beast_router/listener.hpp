#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/system/error_code.hpp>
#include <memory>
#include <string_view>

#include "base/config.hpp"
#include "base/strand_stream.hpp"
#include "common/utility.hpp"

#define LISTENER_TEMPLATE_ATTRIBUTES Protocol, Acceptor, Socket, Endpoint

namespace beast_router {

/// Listens and accepts the incoming connections.
/**
 * The main class which listens and accepts the incoming connections.
 * Interaction is done by using the respective handlers such as on_accept_type
 and on_error_type.
 * Once the connection is accepted, the on_accept_type is invoked; in case of
 error the on_error_type is called.
 *
 * ### Example
 * ```cpp
 * http_listener::on_error_type on_error = [](boost::system::error_code ec,
 std::string_view v) {
 *      if (ec == boost::system::errc::address_in_use ||
 *          ec == boost::system::errc::permission_denied)
 *          g_ioc.stop();
 *  };

 *  http_listener::on_accept_type on_accept =
 [&on_error](http_listener::socket_type socket) {
 *      server_session::recv(std::move(socket), g_router, 5s, on_error);
 *  };

 *  const auto address = boost::asio::ip::address_v4::any();
 *  const auto port = static_cast<unsigned short>(8081);

 *  http_listener::launch(g_ioc, {address, port}, on_accept, on_error);
 * ```
 */
template <class Protocol = boost::asio::ip::tcp,
    class Acceptor = boost::asio::basic_socket_acceptor<Protocol>,
    class Socket = boost::asio::basic_stream_socket<Protocol>,
    template <typename> class Endpoint = boost::asio::ip::basic_endpoint>
class listener : public base::strand_stream,
                 public std::enable_shared_from_this<
                     listener<LISTENER_TEMPLATE_ATTRIBUTES>> {
public:
    /// The self type
    using self_type = listener<LISTENER_TEMPLATE_ATTRIBUTES>;

    /// The protocol type associated with the endpoint
    using protocol_type = Protocol;

    /// the acceptor type
    using acceptor_type = Acceptor;

    /// The socket type
    using socket_type = Socket;

    /// The endpoint type
    using endpoint_type = Endpoint<protocol_type>;

    /// The on accept callback type
    using on_accept_type = std::function<void(socket_type)>;

    /// The on error callback type
    using on_error_type = std::function<void(boost::system::error_code, std::string_view)>;

    /// Constructor
    explicit listener(boost::asio::io_context& ctx, on_accept_type&& on_accept);

    /// Constructor
    explicit listener(boost::asio::io_context& ctx, on_accept_type&& on_accept,
        on_error_type&& on_error);

    /// The factory method which creates `self_type` and starts listening and
    /// accepts new connections
    /**
     * Creates an instance of `self_type` and starts listening by calling loop()
     * method
     *
     * @param ctx A reference to the `boost::asio::io_context`
     * @param endpoint A const reference to the endpoint_type
     * @param on_action A list of actions suitable for the self construction i.e.
     * `on_accept`, `on_error` signatures
     * @returns void
     */
    template <
        class... OnAction,
#if not ROUTER_DOXYGEN
        std::enable_if_t<utility::is_class_creatable_v<
                             self_type, boost::asio::io_context&, OnAction...>,
            bool>
        = true
#endif
        >
    static void launch(boost::asio::io_context& ctx,
        const endpoint_type& endpoint, OnAction&&... on_action)
    {
        std::make_shared<self_type>(ctx, std::forward<OnAction>(on_action)...)
            ->loop(endpoint);
    }

protected:
    /// Starts a loop on the given endpoint
    /**
     * @param endpoint
     * @returns void
     */
    void loop(const endpoint_type& endpoint);

    /// An async accept method. Passes on_accept() as an internal callback which
    /// triggers on new connection
    void do_accept();

    /// An internal on_accept callback and passes the event to on_spawn_method
    /// through the loop
    void on_accept(boost::system::error_code ec, socket_type socket);

    /// Calls the given user specified on_accept callback
    void on_spawn_connect(boost::system::error_code ec, socket_type& socket);

private:
    acceptor_type m_acceptor;
    boost::asio::io_context& m_io_ctx;
    on_accept_type m_on_accept;
    on_error_type m_on_error;
    endpoint_type m_endpoint;
};

/// Default http listener
using http_listener = listener<>;

} // namespace beast_router

#include "impl/listener.ipp"
