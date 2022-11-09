#pragma once

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

#include "base/conn_queue.hpp"
#include "base/dispatcher.hpp"
#include "base/lockable.hpp"
#include "base/storage.hpp"
#include "base/strand_stream.hpp"
#include "common/connection.hpp"
#include "common/timer.hpp"
#include "router.hpp"

#define SESSION_TEMPLATE_ATTRIBUTES IsRequest, Body, Buffer, Protocol, Socket

namespace beast_router {

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
 *
 */
template <bool IsRequest, class Body = boost::beast::http::string_body,
          class Buffer = boost::beast::flat_buffer,
          class Protocol = boost::asio::ip::tcp,
          class Socket = boost::asio::basic_stream_socket<Protocol>>
class session {
  class impl;

 public:
  template <class>
  class context;

  /// Indicates if session serves as a server or a client
#if ROUTER_DOXYGEN
  using is_request = std::integral_constant<bool, IsRequest>;
#else
  using is_request =
      std::conditional_t<IsRequest, std::true_type, std::false_type>;
#endif

  /// The self type
  using self_type = session<SESSION_TEMPLATE_ATTRIBUTES>;

  /// The body type associated with the message_type
  using body_type = Body;

  /// The message type and depends on the value of @ref is_request
#if ROUTER_DOXYGEN
  using message_type =
      boost::beast::http::message<is_request, body_type, Fields>;
#else
  using message_type =
      std::conditional_t<IsRequest, boost::beast::http::request<body_type>,
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
  using connection_type =
      connection<socket_type, base::strand_stream::asio_type>;

  /// The timer type
  using timer_type =
      timer<base::strand_stream::asio_type, boost::asio::steady_timer>;

  /// The timer duration type used for timeouts
  using timer_duration_type = typename timer_type::duration_type;

  /// The on_error callback type
  using on_error_type =
      std::function<void(boost::system::error_code, std::string_view)>;

  /// The shutdown type
  using shutdown_type = typename socket_type::shutdown_type;

  /// The method type references onto `boost::beast::http::verb`
  using method_type = boost::beast::http::verb;

  /// Typedef referencing to the Router type
  using router_type = router<self_type>;

  /// Typedef definition of the connection queue
  using conn_queue_type = base::conn_queue<impl_type>;

  /// Typdef definition of the dispatcher
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
  static context_type recv(socket_type &&socket, const router_type &router,
                           OnAction &&...on_action);

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
  template <class TimeDuration, class... OnAction>
  static context_type recv(socket_type &&socket, const router_type &router,
                           TimeDuration &&duration, OnAction &&...on_action);

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
  template <class Request, class... OnAction>
  static context_type send(socket_type &&socket, Request &&request,
                           const router_type &router, OnAction &&...on_action);

 private:
  template <class... OnAction>
  static context_type init_context(socket_type &&socket,
                                   const router_type &router,
                                   OnAction &&...on_action);

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

    explicit impl(socket_type &&socket, buffer_type &&buffer,
                  const router_type &router, const on_error_type &on_error);

    [[nodiscard]] self_type &recv();
    [[nodiscard]] self_type &recv(timer_duration_type duration);

    template <class Message>
    [[nodiscard]] self_type &send(Message &&message,
                                  timer_duration_type duration);

   private:
    void do_timer(timer_duration_type duraion);
    void on_timer(boost::system::error_code ec);

    void do_read();
    void on_read(boost::system::error_code ec, size_t bytes_transferred);

    void do_eof(shutdown_type type);

    template <class Message>
    void do_process_request(Message &&message);

    template <bool IsMessageRequest, class MessageBody, class Fields>
    void do_write(boost::beast::http::message<IsMessageRequest, MessageBody,
                                              Fields> &message);
    void on_write(boost::system::error_code ec, std::size_t bytes_transferred,
                  bool close);

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
  class context {
    static_assert(
        std::is_base_of_v<
            boost::asio::strand<boost::asio::system_timer::executor_type>,
            Impl>,
        "context requirements are not met");

   public:
    /// Constructor
    context(Impl &impl);

    /// The method receives a data send by the socket
    /**
     * @returns void
     */
    void recv();

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
    recv(TimeDuration &&duration);

    /// The overloaded method does send data back to client
    /**
     * @param message The messge type associated with the Body
     * @returns void
     */
    template <class Message>
    void send(Message &&message) const;

    /// The overloaded method does send data back to client within the timeout
    /**
     * @param message The message type associated with the Body
     * @param duration A time duration used by the timer
     * @returns void
     */
    template <class Message, class TimeDuration>
    void send(Message &&message, TimeDuration &&duration) const;

    /// Obtains the state of the connection
    /**
     * @returns bool
     */
    [[nodiscard]] bool is_open() const;

    /// Obtains the user data; lvalue reference context
    /**
     * @returns Type reference
     */
    template <class Type>
    [[nodiscard]] Type &get_user_data() &;

    /// Obtains the user data; rvalue reference context
    /**
     * @returns Type rvalue eference
     */
    template <class Type>
    [[nodiscard]] Type &&get_user_data() &&;

    /// Sets user data
    /**
     * @param data A data type to store
     * @returns void
     */
    template <class Type>
    void set_user_data(Type &&data);

   protected:
    template <class Message, class TimeDuration>
    void do_send(Message &&message, TimeDuration &&duration) const;

   private:
    std::shared_ptr<Impl> m_impl;
    std::any m_user_data;
  };
};

/// Default http server session
using server_session = session<true>;

/// Default http client sessionl
using client_session = session<false>;

}  // namespace beast_router

#include "impl/session.ipp"
