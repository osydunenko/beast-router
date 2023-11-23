#pragma once

#include "../../base/config.hpp"
#include "../../base/connection.hpp"
#include <boost/asio/ssl.hpp>

#define SSL_CONNECTION_TEMPLATE_ATTRIBUTES Stream, CompletionExecutor

ROUTER_SSL_NAMESPACE_BEGIN()

/// Encapsulates the ssl connection related functionality
template <class Stream, class CompletionExecutor>
class connection
    : public base::connection<connection<Stream, CompletionExecutor>,
          CompletionExecutor> {
public:
    /// Typedef for is ssl
    using is_ssl_context = std::true_type;

    /// The self type
    using self_type = connection<SSL_CONNECTION_TEMPLATE_ATTRIBUTES>;

    /// The ssl stream type
    using stream_type = boost::asio::ssl::stream<Stream>;

    /// The shutdown type
    using shutdown_type = typename Stream::shutdown_type;

    /// The handshake type
    using handshake_type = boost::asio::ssl::stream_base::handshake_type;

    /// Constructor
    explicit connection(Stream&& stream, boost::asio::ssl::context& ctx, const CompletionExecutor& executor);

    /// Destructor
    ~connection() override;

    /// Asynchronous handshake
    /**
     * This function is used to asynchronously perform an SSL handshake on the
     * stream and returns immediately
     *
     * @param type The type of handshaking to be performed
     * @param func The handler to be called when the handshake operation
     * completes. The equivalent function signature of the handler must be as the
     * following:
     * @code
     * handler(const boost::system::error_code &error);
     * @endcode
     * @returns void
     */
    template <class Function>
    void async_handshake(handshake_type type, Function&& func);

    /// Shutes down the connection
    /**
     * @param type The shutdown parameter
     * @returns error_code
     */
    boost::beast::error_code shutdown(shutdown_type type);

    /// Closes the connection
    /**
     * @returns error_code
     */
    boost::beast::error_code close();

    /// Obtains the connection state
    /**
     * @returns bool
     */
    ROUTER_DECL bool is_open() const;

    /// Obtains the connection state
    /**
     * @returns bool
     */
    // bool is_open() const;

    /// Returns the stream and releases the ownership
    /**
     * @returns stream_type
     */
    stream_type release();

    /// Obtains the reference to the stream
    /**
     * @returns stream_type
     */
    stream_type& stream();

private:
    stream_type m_stream;
};

ROUTER_SSL_NAMESPACE_END()

#include "impl/connection.ipp"
