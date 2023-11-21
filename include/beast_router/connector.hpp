#pragma once

#include "base/config.hpp"
#include "base/strand_stream.hpp"
#include "common/connection.hpp"
#include "common/utility.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>
#include <functional>
#include <memory>
#include <string_view>

ROUTER_NAMESPACE_BEGIN()

/// Makes a client connection to the given host
template <class Protocol, class Resolver,
    class Socket, template <typename> class Endpoint>
class connector : public base::strand_stream,
                  public std::enable_shared_from_this<
                      connector<Protocol, Resolver, Socket, Endpoint>> {
public:
    /// The self type
    using self_type = connector<Protocol, Resolver, Socket, Endpoint>;

    /// The protocol type
    using protocol_type = Protocol;

    /// The socket type
    using socket_type = Socket;

    /// The connector type
    using resolver_type = Resolver;

    /// The endpoint type
    using endpoint_type = Endpoint<protocol_type>;

    /// On Connect callback type
    using on_connect_type = std::function<void(socket_type)>;

    /// On Error callback type
    using on_error_type = std::function<void(boost::system::error_code, std::string_view)>;

    /// The results type
    using results_type = typename resolver_type::results_type;

    /// The connector ptr type
    using connector_ptr_type = std::shared_ptr<self_type>;

    /// Constructor (disallowed)
    connector(const connector& srv) = delete;

    /// Assignment (disallowed)
    self_type& operator=(const connector&) = delete;

    /// Constructor (default)
    connector(connector&&) = default;

    /// Assignment (default)
    self_type& operator=(connector&&) = default;

    /// Destructor
    ~connector() = default;

    /// The connection factory method
    /**
     * @param ctx The boost context
     * @param address String representation of the target address
     * @param port String representation of the target port
     * @param on_action A pack of callbacks suitable for the `this` object
     * creation
     * @returns `self_type::connector_ptr_type`
     */
    template <class EventLoop, class... OnAction>
    static auto connect(EventLoop& event_loop,
        std::string_view address, std::string_view port, OnAction&&... on_action)
        -> decltype(self_type { static_cast<boost::asio::io_context&>(event_loop), std::declval<OnAction>()... }, connector_ptr_type())
    {
        struct enable_make_shared : public self_type {
            enable_make_shared(boost::asio::io_context& ctx, OnAction&&... on_actions)
                : self_type { ctx, std::forward<OnAction>(on_actions)... }
            {
            }
        };
        auto ret = std::make_shared<enable_make_shared>(static_cast<boost::asio::io_context&>(event_loop), std::forward<OnAction>(on_action)...);
        ret->do_resolve(address, port);
        return ret;
    }

protected:
    /// Constructor
    explicit connector(boost::asio::io_context& ctx,
        on_connect_type&& on_connect);

    /// Constructor
    explicit connector(boost::asio::io_context& ctx, on_connect_type&& on_connect,
        on_error_type&& on_error);

    /// The method does the endpoint resolution
    /**
     * @param address String representation of the target address
     * @param port String representation of the target port
     * @returns void
     */
    void do_resolve(std::string_view address, std::string_view port);

    /// The On Resolve slot
    /**
     * @param ec An error code
     * @param results The endpoint result
     * @returns void
     */
    void on_resolve(boost::beast::error_code ec, results_type results);

    /// The On Connect slot which calls a user slot
    /**
     * @param ec An error code
     * @param ep The endpoint result
     * @return void
     */
    void on_connect(boost::beast::error_code ec,
        typename results_type::endpoint_type ep);

private:
    using connection_type = connection<socket_type, base::strand_stream::asio_type>;

    resolver_type m_resolver;
    on_connect_type m_on_connect;
    on_error_type m_on_error;
    connection_type m_connection;
};

/// Default connector type
using http_connector_type = connector<boost::asio::ip::tcp,
    boost::asio::ip::tcp::resolver,
    boost::asio::basic_stream_socket<boost::asio::ip::tcp>,
    boost::asio::ip::basic_endpoint>;

ROUTER_NAMESPACE_END()

#include "impl/connector.ipp"
