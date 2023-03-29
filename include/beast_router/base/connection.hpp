#pragma once

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>

#include "config.hpp"

#define BASE_CONNECTION_TEMPLATE_ATTRIBUTES Derived, CompletionExecutor

namespace beast_router {
namespace base {

    template <class Derived, class CompletionExecutor>
    class connection : private boost::noncopyable {
        static_assert(boost::asio::is_executor<CompletionExecutor>::value,
            "connection requirements are not met");

        ROUTER_DECL Derived& derived() { return static_cast<Derived&>(*this); }

    public:
        /// Contructor
        connection(const CompletionExecutor& executor);

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

} // namespace base
} // namespace beast_router

#include "impl/connection.ipp"
