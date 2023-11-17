#pragma once

#include "router.hpp"
#include <boost/asio/ip/tcp.hpp>

namespace beast_router {

/// Encapsulates the server session handling associated
/**
 * The final class which creates and delegates handling of all 
 * the active sessions and dispatch them through the event loop. 
 * The class is associated within the following attributes:
 *
 * @li Session -- defines the session handler
 * @li Listener -- defines the listener handler
 * @li EventLoop -- the event loop to be associated within the server
 *
 */
template <class Session, class Listener, class EventLoop>
struct server final {

    /// The tag is used to bind on Get method
    struct get_t { };

    /// The tag is used to bind on Post method
    struct post_t { };

    /// The tag is used to bind on Put method
    struct put_t { };

    /// The tag is used to bind on Delete method
    struct delete_t { };

    /// The session type
    using session_type = Session;

    /// The listener type
    using listener_type = Listener;

    /// The event loop type
    using event_loop_type = EventLoop;

    /// The router type
    using router_type = router<Session>;

    /// The threads num type
    using threads_num_type = typename event_loop_type::threads_num_type;

    /// The self type
    using self_type = server<session_type, listener_type, event_loop_type>;

    /// The address type
    using address_type = boost::asio::ip::address;

    /// The listening port type
    using port_type = unsigned short;

    /// The duration type used by timeouts
    using duration_type = std::chrono::seconds;

    /// The endpoint type
    using endpoint_type = boost::asio::ip::basic_endpoint<typename session_type::protocol_type>;

    /// Constructor
    explicit server(endpoint_type endpoint = { boost::asio::ip::address_v4::any(), 8080 });

    /// Constructor
    explicit server(threads_num_type thrs, endpoint_type endpoint = { boost::asio::ip::address_v4::any(), 8080 });

    /// Constructor (default)
    server(const server&) = default;

    /// Assignment (default)
    self_type& operator=(const server&) = default;

    /// Constructor (default)
    server(server&&) = default;

    /// Assignment (default)
    self_type& operator=(server&&) = default;

    /// Destructor
    ~server() = default;

    /// The method executes the routing configuration and starts the event loop
    /**
     * @returns int
     */
    [[nodiscard]] ROUTER_DECL int exec();

    /// The method stops all the execution activities
    ROUTER_DECL void stop();

    /// Returns the current timeout on data receive
    /**
     * @returns self_type::duration_type
     */
    ROUTER_DECL duration_type receive_timeout() const { return m_recv_timeout; }

    /// A proxy function for the routing
    template <class Tag, class... OnRequest>
    ROUTER_DECL self_type& on(const std::string& path, OnRequest&&... actions);

private:
    using event_loop_ptr_type = typename event_loop_type::event_loop_ptr_type;

    endpoint_type m_endpoint;
    event_loop_ptr_type m_event_loop;
    router_type m_router;
    duration_type m_recv_timeout;
};

} // namespace beast_router

#include "impl/server.ipp"