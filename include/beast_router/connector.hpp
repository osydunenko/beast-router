#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>
#include <functional>
#include <memory>
#include <string_view>

#include "base/config.hpp"
#include "base/strand_stream.hpp"
#include "common/connection.hpp"
#include "common/utility.hpp"

#define CONNECTOR_TEMPLATE_ATTRIBUTES Protocol, Resolver, Socket, Endpoint

namespace beast_router {

/// Makes a client connection to the given host
template <class Protocol = boost::asio::ip::tcp,
          class Resolver = typename Protocol::resolver,
          class Socket = boost::asio::basic_stream_socket<Protocol>,
          template <typename> class Endpoint = boost::asio::ip::basic_endpoint>
class connector : public base::strand_stream,
                  public std::enable_shared_from_this<
                      connector<CONNECTOR_TEMPLATE_ATTRIBUTES>> {
 public:
  /// The self type
  using self_type = connector<CONNECTOR_TEMPLATE_ATTRIBUTES>;

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
  using on_error_type =
      std::function<void(boost::system::error_code, std::string_view)>;

  /// The results type
  using results_type = typename resolver_type::results_type;

  /// Constructor
  explicit connector(boost::asio::io_context &ctx,
                     on_connect_type &&on_connect);

  /// Constructor
  explicit connector(boost::asio::io_context &ctx, on_connect_type &&on_connect,
                     on_error_type &&on_error);

  /// The connection factory method
  /**
   * @param ctx The boost context
   * @param address String representation of the target address
   * @param port String representation of the target port
   * @param on_action A pack of callbacks suitable for the `this` object
   * creation
   * @returns void
   */
  template <
      class... OnAction,
#if not ROUTER_DOXYGEN
      std::enable_if_t<utility::is_class_creatable_v<
                           self_type, boost::asio::io_context &, OnAction...>,
                       bool> = true
#endif
      >
  static void connect(boost::asio::io_context &ctx, std::string_view address,
                      std::string_view port, OnAction &&...on_action) {
    std::make_shared<self_type>(ctx, std::forward<OnAction>(on_action)...)
        ->do_resolve(address, port);
  }

 protected:
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
  using connection_type =
      connection<socket_type, base::strand_stream::asio_type>;

  resolver_type m_resolver;
  on_connect_type m_on_connect;
  on_error_type m_on_error;
  connection_type m_connection;
};

using plain_connector = connector<>;

}  // namespace beast_router

#include "impl/connector.ipp"
