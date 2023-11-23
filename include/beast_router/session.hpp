#pragma once

#include "base/config.hpp"
#include "base/conn_queue.hpp"
#include "base/dispatcher.hpp"
#include "base/lockable.hpp"
#include "base/storage.hpp"
#include "base/strand_stream.hpp"
#include "common/connection.hpp"
#include "common/timer.hpp"
#include "router.hpp"
#include <algorithm>
#include <any>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/string_body.hpp>
#include <string_view>
#include <type_traits>
#include <utility>

ROUTER_NAMESPACE_BEGIN()
template <bool, class, class, class, class, class>
class session;

/// Default http server session type
using http_server_type = session<true,
    boost::beast::http::string_body,
    boost::beast::flat_buffer,
    boost::asio::ip::tcp,
    boost::asio::basic_stream_socket<boost::asio::ip::tcp>,
    connection<boost::asio::basic_stream_socket<boost::asio::ip::tcp>,
        base::strand_stream::asio_type>>;

/// Default http client session type
using http_client_type = session<false,
    boost::beast::http::string_body,
    boost::beast::flat_buffer,
    boost::asio::ip::tcp,
    boost::asio::basic_stream_socket<boost::asio::ip::tcp>,
    connection<boost::asio::basic_stream_socket<boost::asio::ip::tcp>,
        base::strand_stream::asio_type>>;
ROUTER_NAMESPACE_END()

#if defined(LINK_SSL)
#include "common/ssl/connection.hpp"
ROUTER_SSL_NAMESPACE_BEGIN()
/// Default tls http server session type
using http_server_type = session<true,
    boost::beast::http::string_body,
    boost::beast::flat_buffer,
    boost::asio::ip::tcp,
    boost::asio::basic_stream_socket<boost::asio::ip::tcp>,
    ssl::connection<boost::asio::basic_stream_socket<boost::asio::ip::tcp>,
        base::strand_stream::asio_type>>;

/// Default tls http client session type
using http_client_type = session<false,
    boost::beast::http::string_body,
    boost::beast::flat_buffer,
    boost::asio::ip::tcp,
    boost::asio::basic_stream_socket<boost::asio::ip::tcp>,
    ssl::connection<boost::asio::basic_stream_socket<boost::asio::ip::tcp>,
        base::strand_stream::asio_type>>;
ROUTER_SSL_NAMESPACE_END()
#endif

ROUTER_NAMESPACE_BEGIN()

/// Encapsulates the sessions handling associated with a buffer
/**
 * The common class which handles all the active sessions and
 * dispatch them through the event loop. The class is associated
 * within the following attributes:
 *
 * @li IsRequest -- Defines belonging to the mode of a session a.k.a. server
 * versus client
 * @li Body -- configures the body type used for the incoming messages
 * @li Buffer -- A flat buffer which uses the default allocator
 * @li Protocol -- Defines a protocl used for
 * @li Socket -- Defines a sicket type
 */
template <bool IsRequest, class Body, class Buffer,
    class Protocol, class Socket, class Connection>
class session final {
    class impl;

    static constexpr bool is_ssl_context_v = Connection::is_ssl_context::value;

public:
    template <class>
    class context;

    /// Indicates if session serves as a server or a client
#if ROUTER_DOXYGEN
    using is_request = std::integral_constant<bool, IsRequest>;
#else
    using is_request = std::conditional_t<IsRequest, std::true_type, std::false_type>;
#endif

    /// The self type
    using self_type = session<IsRequest, Body, Buffer, Protocol, Socket, Connection>;

    /// The body type associated with the message_type
    using body_type = Body;

    /// The message type and depends on the value of @ref is_request
#if ROUTER_DOXYGEN
    using message_type = boost::beast::http::message<is_request, body_type, Fields>;
#else
    using message_type = std::conditional_t<IsRequest, boost::beast::http::request<body_type>,
        boost::beast::http::response<body_type>>;
#endif

    /// The buffer type
    using buffer_type = Buffer;

    /// The protocol type
    using protocol_type = Protocol;

    /// The socket type
    using socket_type = Socket;

    /// The mutex type
    using mutex_type = base::lockable::mutex_type;

    /// The internal `impl` type
    using impl_type = impl;

    /// The context type
    using context_type = context<impl_type>;

    /// The connection type
    using connection_type = Connection;

    /// The stream type
    using stream_type = typename connection_type::stream_type;

    /// The timer type
    using timer_type = timer<base::strand_stream::asio_type, boost::asio::steady_timer>;

    /// The timer duration type used for timeouts
    using timer_duration_type = typename timer_type::duration_type;

    /// The on_error callback type
    using on_error_type = std::function<void(boost::system::error_code, std::string_view)>;

    /// The shutdown type
    using shutdown_type = typename socket_type::shutdown_type;

    /// The method type references onto `boost::beast::http::verb`
    using method_type = boost::beast::http::verb;

    /// Typedef referencing to the Router type
    using router_type = router<self_type>;

    /// Typedef definition of the connection queue
    using conn_queue_type = base::conn_queue<impl_type>;

    /// Typedef definition of the dispatcher
    using dispatcher_type = base::dispatcher<self_type>;

    /// The method for receiving data
    /**
     * The method receives data send by a connection
     *
     * @param socket An rvalue reference to the socket
     * @param router A const reference to the Router
     * @param on_action A list of callbacks
     * @returns context_type
     */
    template <class... OnAction>
    static auto recv(socket_type&& socket, const router_type& router,
        OnAction&&... on_action)
        -> decltype(not is_ssl_context_v, context_type())
    {
        static_assert(IsRequest, "session::recv requirements are not met");
        context_type ctx = init_context(std::move(socket), router,
            std::forward<OnAction>(on_action)...);
        ctx.recv();
        return ctx;
    }

    /// The method for receiving data whithin the given timeout
    /**
     * The method receives data send by a connection whithin the given duration
     * by creating a timer
     *
     * @param socket An rvalue reference to the socket
     * @param router A const reference to the Router
     * @param duration A duration for handling the timeout
     * @param on_action A list of callbacks
     * @returns context_type
     */
    template <class TimeDuration,
        class... OnAction>
    static auto recv(std::piecewise_construct_t, socket_type&& socket, const router_type& router,
        TimeDuration&& duration, OnAction&&... on_action)
        -> decltype(not is_ssl_context_v, context_type())
    {
        static_assert(IsRequest, "session::recv requirements are not met");
        context_type ctx = init_context(std::move(socket), router,
            std::forward<OnAction>(on_action)...);
        ctx.recv(std::forward<TimeDuration>(duration));
        return ctx;
    }

    /// The method for sending data
    /**
     * The method does send data by the using the corresponding socket
     *
     * @param socket An rvalue reference to the socket
     * @param request The request message to be sent
     * @param router A const reference to the router
     * @param on_action A list of callbacks
     * @returns context_type
     */
    template <class Request,
        class... OnAction>
    static auto send(socket_type&& socket, Request&& request,
        const router_type& router, OnAction&&... on_action)
        -> decltype(not is_ssl_context_v, context_type())
    {
        static_assert(!IsRequest, "session::send requirements are not met");
        context_type ctx = init_context(std::move(socket), router,
            std::forward<OnAction>(on_action)...);
        ctx.send(std::forward<Request>(request));
        return ctx;
    }

    /// The method for sending data
    /**
     * The method does send data by the using the corresponding socket
     *
     * @param socket An rvalue reference to the socket
     * @param request The request message to be sent
     * @param router A const reference to the router
     * @param duration A duration for handling the timeout
     * @param on_action A list of callbacks
     * @returns context_type
     */
    template <class Request,
        class TimeDuration,
        class... OnAction>
    static auto send(std::piecewise_construct_t, socket_type&& socket, Request&& request,
        const router_type& router, TimeDuration&& duration, OnAction&&... on_action)
        -> decltype(not is_ssl_context_v, context_type())
    {
        static_assert(!IsRequest, "session::send requirements are not met");
        context_type ctx = init_context(std::move(socket), router,
            std::forward<OnAction>(on_action)...);
        ctx.send(std::forward<Request>(request), std::forward<TimeDuration>(duration));
        return ctx;
    }

#if defined(LINK_SSL)
    template <class... OnAction>
    static auto recv(boost::asio::ssl::context& ssl_ctx,
        socket_type&& socket, const router_type& router, OnAction&&... on_action)
        -> decltype(is_ssl_context_v, context_type())
    {
        static_assert(IsRequest, "session::recv requirements are not met");
        context_type ctx = init_context(ssl_ctx, std::move(socket), router,
            std::forward<OnAction>(on_action)...);
        ctx.handshake([](context_type& ctx) { ctx.recv(); });
        return ctx;
    }

    template <class TimeDuration, class... OnAction>
    static auto recv(std::piecewise_construct_t, boost::asio::ssl::context& ssl_ctx,
        socket_type&& socket, const router_type& router, TimeDuration&& duration,
        OnAction&&... on_action)
        -> decltype(is_ssl_context_v, context_type())
    {
        static_assert(IsRequest, "session::recv requirements are not met");
        context_type ctx = init_context(ssl_ctx, std::move(socket), router,
            std::forward<OnAction>(on_action)...);
        ctx.handshake([](context_type& ctx) { ctx.recv(); });
        return ctx;
    }
#endif

private:
    template <class... OnAction>
    static context_type init_context(socket_type&& socket,
        const router_type& router,
        OnAction&&... on_action);

#if defined(LINK_SSL)
    template <class... OnAction>
    static context_type init_context(boost::asio::ssl::context& ssl_ctx,
        socket_type&& socket,
        const router_type& router,
        OnAction&&... on_action);
#endif

    class impl : public base::strand_stream,
                 public std::enable_shared_from_this<impl> {
        template <class>
        friend class context;

        template <class>
        friend class base::conn_queue;

        using request_parser_type = boost::beast::http::request_parser<body_type>;
        using response_parser_type = boost::beast::http::response_parser<body_type>;

    public:
        using self_type = impl;

        using parser_type = std::conditional_t<IsRequest, request_parser_type,
            response_parser_type>;

        explicit impl(socket_type&& socket, buffer_type&& buffer,
            const router_type& router, const on_error_type& on_error);

#if defined(LINK_SSL)
        explicit impl(boost::asio::ssl::context& ssl_ctx, socket_type&& socket, buffer_type&& buffer,
            const router_type& router, const on_error_type& on_error);
#endif

        self_type& recv();
        self_type& recv(timer_duration_type duration);

        template <class Message>
        self_type& send(Message&& message);

        template <class Message>
        self_type& send(Message&& message,
            timer_duration_type duration);

    private:
        void do_timer(timer_duration_type duraion);
        void on_timer(boost::system::error_code ec);

        void do_read();
        void on_read(boost::system::error_code ec, size_t bytes_transferred);

        void do_eof(shutdown_type type);

        template <class Message>
        void do_process_request(Message&& message);

        template <bool IsMessageRequest, class MessageBody, class Fields>
        void do_write(boost::beast::http::message<IsMessageRequest, MessageBody,
            Fields>& message);

        void on_write(boost::system::error_code ec, std::size_t bytes_transferred,
            bool close);

#if defined(LINK_SSL)
        template <class Func>
        self_type& do_handshake(Func&& func);

        template <class Func>
        self_type& do_handshake(Func&& func, timer_duration_type duration);

        void on_handshake(boost::system::error_code ec);
#endif

    private:
        connection_type m_connection;
        timer_type m_timer;
        buffer_type m_buffer;
        on_error_type m_on_error;
        conn_queue_type m_queue;
        std::any m_serializer;
        parser_type m_parser;
        dispatcher_type m_dispatcher;
    };

public:
    /// The Context class
    /**
     * The main class which is passed as a parameter to the user
     * and used for the interaction whithin the connection.
     */
    template <class Impl>
    class context final {
        static_assert(
            std::is_base_of_v<
                boost::asio::strand<boost::asio::system_timer::executor_type>,
                Impl>,
            "context requirements are not met");

        friend class session;

    public:
        /// Constructor
        context(Impl& impl);

        /// The method receives a data send by the socket
        /**
         * @returns void
         */
        ROUTER_DECL void recv();

        /// The method receives a data send by the socket whithin the timeout
        /**
         * @param duration A time duration used by the timer
         * @returns void
         */
        template <class TimeDuration>
#if ROUTER_DOXYGEN
        void
#else
        typename std::enable_if_t<utility::is_chrono_duration_v<TimeDuration>>
#endif
            ROUTER_DECL
            recv(TimeDuration&& duration);

        /// The overloaded method does send data back to client
        /**
         * @param message The messge type associated with the Body
         * @returns void
         */
        template <class Message>
        ROUTER_DECL void send(Message&& message) const;

        /// The overloaded method does send data back to client within the timeout
        /**
         * @param message The message type associated with the Body
         * @param duration A time duration used by the timer
         * @returns void
         */
        template <class Message, class TimeDuration>
        ROUTER_DECL void send(Message&& message, TimeDuration&& duration) const;

        /// The method executes handshake in SSL context
        /**
         * @param func Callback on success accepts `self_type` as a reference
         * @returns void
         */
        template <class Func>
        ROUTER_DECL void handshake(Func&& func) const;

        /// The method executes handshake in SSL context
        /**
         * @param func Callback on success accepts `self_type` as a reference
         * @param duration A time duration used by the timer
         * @returns void
         */
        template <class Func, class TimeDuration>
        ROUTER_DECL void handshake(Func&& func, TimeDuration&& duration) const;

        /// Obtains the state of the connection
        /**
         * @returns bool
         */
        [[nodiscard]] ROUTER_DECL bool is_open() const;

        /// Obtains the user data; lvalue reference context
        /**
         * @returns Type reference
         */
        template <class Type>
        [[nodiscard]] ROUTER_DECL Type& get_user_data() &;

        /// Obtains the user data; rvalue reference context
        /**
         * @returns Type rvalue eference
         */
        template <class Type>
        [[nodiscard]] ROUTER_DECL Type&& get_user_data() &&;

        /// Sets user data
        /**
         * @param data A data type to store
         * @returns void
         */
        template <class Type>
        ROUTER_DECL void set_user_data(Type&& data);

        /// Obtains the current associated stream
        /**
         * @returns Reference of `connection_type::stream_type`
         */
        ROUTER_DECL typename connection_type::stream_type& get_stream();

    private:
        context();
        std::shared_ptr<Impl> m_impl;
        std::any m_user_data;
    };
};

ROUTER_NAMESPACE_END()

#include "impl/session.ipp"
