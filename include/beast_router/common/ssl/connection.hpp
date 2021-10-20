#pragma once

#include "../../base/connection.hpp"

#include <boost/beast/ssl.hpp>

#define SSL_CONNECTION_TEMPLATE_ATTRIBUTES \
    Stream, CompletionExecutor

namespace beast_router {
namespace ssl {

/// Encapsulates the ssl connection related functionality
/**
 * @note The class is neither copyable nor assignment
 */
template <class Stream, class CompletionExecutor>
class connection: public base::connection<
    connection<Stream, CompletionExecutor>, CompletionExecutor>
{
public:
    /// The self type
    using self_type = connection<SSL_CONNECTION_TEMPLATE_ATTRIBUTES>;

    /// The stream type
    using stream_type = Stream;

    /// The SSL stream type
    using ssl_stream_type = boost::beast::ssl_stream<stream_type>;

    /// The shutdown type
    using shutdown_type = typename stream_type::shutdown_type;

    /// The handshake type
    using handshake_type = boost::asio::ssl::stream_base::handshake_type;

    /// Constructor
    explicit connection(Stream &&stream, const CompletionExecutor &executor);

    /// Destructor
    ~connection() override;

    /// Asynchronous handshake
    /**
     * This function is used to asynchronously perform an SSL handshake on the
     * stream and returns immediately
     * 
     * @param type The type of handshaking to be performed
     * @param func The handler to be called when the handshake operation completes.
     * The equivalent function signature of the handler must be as the following:
     * @code
     * handler(const boost::system::error_code &error);
     * @endcode
     * @returns void
     */
    template <class Function>
    void async_handshake(handshake_type type, Function &&func);

    /// Shutes down the connection
    /**
     * @param type The shutdown parameter
     * @returns error_code
     */
    boost::beast::error_code
    shutdown(shutdown_type type);

    /// Closes the connection
    /**
     * @returns error_code
     */
    boost::beast::error_code
    close();

    /// Obtains the connection state
    /**
     * @returns bool
     */
    bool
    is_open() const;

    /// Returns the stream and releases the ownership
    /**
     * @returns stream_type
     */
    stream_type release();

    /// Obtains the reference to the stream
    /**
     * @returns stream_type
     */
    stream_type &stream();

private:
    stream_type m_stream;
};

} // namespace ssl
} // namespace beast_router

#include "impl/connection.ipp"