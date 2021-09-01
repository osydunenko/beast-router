#pragma once

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/read.hpp>

#include "utility.hpp"

#define CONNECTION_TEMPLATE_ATTRIBUTES \
    Socket, CompletionExecutor

namespace beast_router {

/// Encapsulates the connections related functionality
/**
 * @note The class is neither copyable nor assignment
 */
template<class Socket, class CompletionExecutor>
class connection
{
    static_assert(
        boost::asio::is_executor<CompletionExecutor>::value,
        "connection requirements are not met");
public:
    /// The self type
    using self_type = connection<CONNECTION_TEMPLATE_ATTRIBUTES>;

    /// The socket type
    using socket_type = Socket;

    /// The shutdown type
    using shutdown_type = typename socket_type::shutdown_type;

    /// Constructor
    explicit connection(Socket &&socket, const CompletionExecutor &executor);

    /// Constructor (disallowed)
    connection(const self_type &) = delete;

    /// Assignment (disallowed)
    self_type &
    operator=(const self_type &) = delete;

    /// Constructor
    connection(self_type &&) = default;

    /// Assignment
    self_type &
    operator=(self_type &&) = default;

    /// Destructor
    ~connection();

    /// Asynchronous writer
    /**
     * @param serializer A reference to the serializer which is associated with the connection
     * @param func A reference to the callback
     * @returns void
     */
    template <class Function, class Serializer>
    void
    async_write(Serializer &serializer, Function &&func);

    /// Asynchronous reader
    /**
     * @param buffer A reference to the buffer associated with the connection
     * @param parser A reference to the parser associated with the connection
     * @param func A reference to the callback
     * @returns void
     */
    template <class Function, class Buffer, class Parser>
    void
    async_read(Buffer &buffer, Parser &parser, Function &&func);

    /// Asynchronous connection
    /**
     * @param endpoint The endpoints sequence
     * @param func A reference to the callback
     * @returns void
     */
    template <class Function, class EndpointSequence>
    void
    async_connect(const EndpointSequence &endpoints, Function &&func);

    /// Shutdowns a connection
    /**
     * @param type The shutdown parameter
     * @returns error_code
     */
    boost::beast::error_code
    shutdown(shutdown_type type);

    /// Closes a connection
    /**
     * @returns error_code
     */
    boost::beast::error_code
    close();

    /// Obtains a connection status
    /**
     * @returns bool
     */
    bool
    is_open() const;

    /// Returns a socket and releases the ownership
    /**
     * @returns socket
     */
    socket_type release();

private:
    Socket m_socket;
    const CompletionExecutor &m_complition_executor;
};

} // namespace beast_router

#include "impl/connection.ipp"
