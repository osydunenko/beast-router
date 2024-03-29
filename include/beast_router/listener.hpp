#pragma once

#include "base/config.hpp"
#include "base/strand_stream.hpp"
#include "common/utility.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/system/error_code.hpp>
#include <memory>
#include <string_view>

ROUTER_NAMESPACE_BEGIN()

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
 * http_listener_type::on_error_type on_error = [](boost::system::error_code ec,
 std::string_view v) {
 *      if (ec == boost::system::errc::address_in_use ||
 *          ec == boost::system::errc::permission_denied)
 *          g_ioc.stop();
 *  };

 *  http_listener_type::on_accept_type on_accept =
 [&on_error](http_listener_type::socket_type socket) {
 *      http_server_type::recv(std::move(socket), g_router, 5s, on_error);
 *  };

 *  const auto address = boost::asio::ip::address_v4::any();
 *  const auto port = static_cast<unsigned short>(8081);

 *  http_listener_type::launch(g_ioc, {address, port}, on_accept, on_error);
 * ```
 */
template <class Protocol, class Acceptor,
    class Socket, template <typename> class Endpoint>
class listener : public base::strand_stream,
                 public std::enable_shared_from_this<
                     listener<Protocol, Acceptor, Socket, Endpoint>> {
public:
    /// The self type
    using self_type = listener<Protocol, Acceptor, Socket, Endpoint>;

    /// The listener pointer type
    using listener_ptr_type = std::shared_ptr<self_type>;

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

    /// Constructor (disallowed)
    listener(const listener&) = delete;

    /// Assignment (disallowed)
    self_type& operator=(const listener&) = delete;

    /// Constructor (default)
    listener(listener&&) = default;

    /// Assignment (default)
    self_type& operator=(listener&&) = default;

    /// Destructor
    ~listener() = default;

    /// The factory method which creates `self_type` and starts listening and
    /// accepts new connections
    /**
     * Creates an instance of `self_type` and starts listening by calling loop()
     * method
     *
     * @param event_loop A reference to the `event_loop`
     * @param endpoint A const reference to the endpoint_type
     * @param on_action A list of actions suitable for the self construction i.e.
     * `on_accept`, `on_error` signatures
     * @returns `selft_type::listener_ptr_type`
     */
    template <class EventLoop, class... OnAction>
    static auto launch(EventLoop& event_loop,
        const endpoint_type& endpoint, OnAction&&... on_action)
        -> decltype(self_type { static_cast<boost::asio::io_context&>(event_loop), std::declval<OnAction>()... }, listener_ptr_type())
    {
        struct enable_make_shared : public self_type {
            enable_make_shared(boost::asio::io_context& ctx, OnAction&&... on_actions)
                : self_type { ctx, std::forward<OnAction>(on_actions)... }
            {
            }
        };

        auto lstnr = std::make_shared<enable_make_shared>(static_cast<boost::asio::io_context&>(event_loop),
            std::forward<OnAction>(on_action)...);
        lstnr->loop(endpoint);
        return lstnr;
    }

    /// The method closes the associated acceptor
    /**
     * @returns void
     */
    ROUTER_DECL void close()
    {
        m_acceptor.close();
    }

protected:
    /// Constructor
    explicit listener(boost::asio::io_context& ctx, on_accept_type&& on_accept);

    /// Constructor
    explicit listener(boost::asio::io_context& ctx, on_accept_type&& on_accept,
        on_error_type&& on_error);

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

/// Default http listener type
using http_listener_type = listener<boost::asio::ip::tcp,
    boost::asio::basic_socket_acceptor<boost::asio::ip::tcp>,
    boost::asio::basic_stream_socket<boost::asio::ip::tcp>,
    boost::asio::ip::basic_endpoint>;

ROUTER_NAMESPACE_END()

#include "impl/listener.ipp"
