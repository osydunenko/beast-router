#pragma once

#include "config.hpp"
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>

#define BASE_CONNECTION_TEMPLATE_ATTRIBUTES Derived, CompletionExecutor

ROUTER_BASE_NAMESPACE_BEGIN()

template <class Derived, class CompletionExecutor>
class connection {
    static_assert(boost::asio::is_executor<CompletionExecutor>::value,
        "connection requirements are not met");

    ROUTER_DECL Derived& derived() { return static_cast<Derived&>(*this); }

public:
    /// Contructor
    connection(const CompletionExecutor& executor);

    /// Constructor (disallowed)
    connection(connection&) = delete;

    /// Assignment (disallowed)
    connection& operator=(const connection&) = delete;

    /// Constructor (default)
    connection(connection&&) = default;

    /// Assignment (default)
    connection& operator=(connection&&) = default;

    /// Destructor
    virtual ~connection() = default;

    /// Asynchronous connection
    /**
     * This function is used to asynchronously establish a connection
     * on the stream and returns immediately
     *
     * @param endpoint The endpoints sequence used for connection
     * @param func The handler to be called right after the connection is
     * established The equivalent function signature of the handler must be as the
     * folowing:
     * @code
     * handler(boost::system::error_code &error, const EndpoitSequence
     * &endpoints);
     * @endcode
     * @returns void
     */
    template <class Function, class EndpointSequence>
    void async_connect(const EndpointSequence& endpoints, Function&& func);

    /// Asynchronous writer
    /**
     * @param serializer A reference to the serializer which is associated with
     * the connection
     * @param func A reference to the callback
     * @returns void
     */
    template <class Function, class Serializer>
    void async_write(Serializer& serializer, Function&& func);

    /// Asynchronous reader
    /**
     * @param buffer A reference to the buffer associated with the connection
     * @param parser A reference to the parser associated with the connection
     * @param func A reference to the callback
     * @returns void
     */
    template <class Function, class Buffer, class Parser>
    void async_read(Buffer& buffer, Parser& parser, Function&& func);

protected:
    const CompletionExecutor& m_completion_executor;
};

ROUTER_BASE_NAMESPACE_END()

#include "impl/connection.ipp"
