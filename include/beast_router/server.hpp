#pragma once

#include "listener.hpp"
#include "router.hpp"
#include "session.hpp"

namespace beast_router {

/// Encapsulates the server handlers and client sessions
/**
 * The common class which handles all the active sessions and
 * dispatch them through the event loop. The class is associated
 * within the following attributes:
 *
 * @li Session -- Session definition class as defined in session.hpp
 */
template <class Session>
class server {
public:
    /// The self type
    using self_type = server<Session>;

    /// The session type
    using session_type = Session;

    /// The address type
    using address_type = boost::asio::ip::address;

    /// The listening port type
    using port_type = unsigned short;

    /// The duration type used by timeouts
    using duration_type = std::chrono::milliseconds;

    /// The router type
    using router_type = server_session::router_type;

    /// The message type
    using message_type = server_session::message_type;

    /// Constructor
    server();

    /// Constructor (disallowed)
    server(self_type const&) = delete;

    /// Constructor (disallowed)
    server(self_type&&) = delete;

    /// Assignment (disallowed)
    self_type& operator=(self_type const&) = delete;

    /// Assignment (disallowed)
    self_type& operator=(self_type&&) = delete;

    /// Destructor
    ~server() = default;

    /// Sets the address for listening on
    /**
     * @param address The <tt>std::string</tt> type and represents IPv4/6 address
     * @returns void
     */
    ROUTER_DECL void set_address(const std::string& address);

    /// Returns the listening address
    /**
     * @returns std::string
     */
    [[nodiscard]] ROUTER_DECL std::string address() const;

    /// Sets the port for listening on
    /**
     * @param port The <tt>self_type::port_type</tt> type represents the port number
     * @returns void
     */
    ROUTER_DECL void set_port(port_type port);

    /// Returns the current port number
    /**
     * @returns self_type::port_type
     */
    [[nodiscard]] ROUTER_DECL port_type port() const;

    /// Sets the receive timeout
    /**
     * @param timeout The <tt>self_type::duration_type</tt> type represents the current
     * timeout for data receiving from the client connection
     * @returns void
     */
    ROUTER_DECL void set_recv_timeout(duration_type timeout);

    /// Returns the current receive timeout
    /**
     * @returns self_type::duration_type
     */
    [[nodiscard]] ROUTER_DECL duration_type recv_timeout() const;

    /// A proxy function for the routing -- router::get
    template <class... OnRequest>
    ROUTER_DECL self_type& on_get(const std::string& path, OnRequest&&... actions);

    /// A proxy function for the routing -- router::put
    template <class... OnRequest>
    ROUTER_DECL self_type& on_put(const std::string& path, OnRequest&&... actions);

    /// A proxy function for the routing -- router::post
    template <class... OnRequest>
    ROUTER_DECL self_type& on_post(const std::string& path, OnRequest&&... actions);

    /// A proxy function for the routing -- router::delete_
    template <class... OnRequest>
    ROUTER_DECL self_type& on_delete(const std::string& path, OnRequest&&... actions);

    /// The method executes the routing configuration and starts the event loop
    /**
     * @returns int
     */
    ROUTER_DECL int exec();

private:
    router_type m_router;
    address_type m_address;
    port_type m_port;
    duration_type m_recv_timeout;
};

using http_server = server<server_session>;

} // namespace beast_router

#include "impl/server.ipp"