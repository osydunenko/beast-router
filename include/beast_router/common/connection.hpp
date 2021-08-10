#pragma once

#include <boost/asio/bind_executor.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/read.hpp>

#define CONNECTION_TEMPLATE_ATTRIBUTES \
    Socket, CompletionExecutor

namespace beast_router {

/// Encapsulates the connections related functionality
/**
 * @note The class is not copyable and assignment
 */
template<class Socket, class CompletionExecutor>
class connection
{
public:
    /// The self type
    using self_type = connection<CONNECTION_TEMPLATE_ATTRIBUTES>;

    /// The socket type
    using socket_type = Socket;

    /// The shutdown type
    using shutdown_type = typename socket_type::shutdown_type;

    /// Constructor
    /**
     * @param socket An rvalue reference to socket
     * @executor A const reference to the complition executor
     */
    explicit connection(Socket &&socket, const CompletionExecutor &executor);

    /// Copy constructor - deleted
    connection(const self_type &) = delete;

    /// Assignment operator - deleted
    self_type &operator=(const self_type &) = delete;

    /// Move contructor - deleted
    connection(self_type &&) = delete;

    /// Move assignment operator - deleted
    self_type &operator=(self_type &&) = delete;

    /// Destructor
    ~connection();

    /// Asynchronous writer
    /**
     * @param serializer A reference to the serializer which is associated with the connection
     * @param func A universal reference to the callback
     * @returns void
     */
    template <class Function, class Serializer>
    void async_write(Serializer &serializer, Function &&func);

    /// Asynchronous reader
    /**
     * @param buffer A reference to the buffer associated with the connection
     * @param parser A reference to the parser associated with the connection
     * @param func A universal reference to the callback
     * @returns void
     */
    template <class Function, class Buffer, class Parser>
    void async_read(Buffer &buffer, Parser &parser, Function &&func);

    /// Shutdowns a connection
    /**
     * @param type The shutdown parameter
     * @returns error_code
     */
    boost::beast::error_code shutdown(shutdown_type type);

    /// Closes a connection
    /**
     * @returns error_code
     */
    boost::beast::error_code close();

    /// Obtains a connection status
    /**
     * @returns bool
     */
    bool is_open() const;

private:
    Socket m_socket;
    const CompletionExecutor &m_complition_executor;
};

} // namespace beast_router

#include "impl/connection.ipp"
