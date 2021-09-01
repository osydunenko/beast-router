#pragma once

#define CONNECTION_TEMPLATE_DECLARE \
    template<                       \
    class Socket,                   \
    class CompletionExecutor        \
    >

namespace beast_router {

CONNECTION_TEMPLATE_DECLARE
connection<CONNECTION_TEMPLATE_ATTRIBUTES>::connection(Socket &&socket, const CompletionExecutor &executor)
    : m_socket{std::move(socket)}
    , m_complition_executor{executor}
{
}

CONNECTION_TEMPLATE_DECLARE
connection<CONNECTION_TEMPLATE_ATTRIBUTES>::~connection()
{
    if (is_open()) {
        shutdown(shutdown_type::shutdown_both);
    }
}

CONNECTION_TEMPLATE_DECLARE
template<class Function, class Serializer>
void connection<CONNECTION_TEMPLATE_ATTRIBUTES>::async_write(Serializer &serializer, Function &&func)
{
    static_assert(
        std::is_invocable_v<Function, boost::system::error_code, size_t>,
        "connection::async_write/Function requirements are not met");

    boost::beast::http::async_write(m_socket, serializer,
        boost::asio::bind_executor(
            m_complition_executor, std::forward<Function>(func)
        )
    );
}

CONNECTION_TEMPLATE_DECLARE
template <class Function, class Buffer, class Parser>
void connection<CONNECTION_TEMPLATE_ATTRIBUTES>::async_read(Buffer &buffer, Parser &parser, Function &&func)
{
    static_assert(
        utility::is_all_true_v<
            std::is_invocable_v<Function, boost::system::error_code, size_t>,
            boost::asio::is_dynamic_buffer<Buffer>::value
        >, "connection::async_read requirements are not met");

    boost::beast::http::async_read(m_socket, buffer, parser,
        boost::asio::bind_executor(
            m_complition_executor, std::forward<Function>(func)
        )
    );
}

CONNECTION_TEMPLATE_DECLARE
template <class Function, class EndpointSequence>
void connection<CONNECTION_TEMPLATE_ATTRIBUTES>::async_connect(const EndpointSequence &endpoint, Function &&func)
{
    static_assert(
        utility::details::is_all_true<
            std::is_invocable_v<Function, boost::system::error_code,
                typename socket_type::endpoint_type>,
            boost::asio::is_endpoint_sequence<EndpointSequence>::value
        >::value, "connection::async_connect requirements are not met");

    boost::asio::async_connect(m_socket, endpoint,
        boost::asio::bind_executor(
            m_complition_executor,
            std::forward<Function>(func)
        )
    );
}

CONNECTION_TEMPLATE_DECLARE
boost::beast::error_code connection<CONNECTION_TEMPLATE_ATTRIBUTES>::shutdown(shutdown_type type)
{
    auto ec = boost::system::error_code{};
    m_socket.shutdown(type, ec);

    return ec;
}

CONNECTION_TEMPLATE_DECLARE
boost::beast::error_code connection<CONNECTION_TEMPLATE_ATTRIBUTES>::close()
{
    auto ec = boost::system::error_code{};
    m_socket.close(ec);

    return ec;
}

CONNECTION_TEMPLATE_DECLARE
bool connection<CONNECTION_TEMPLATE_ATTRIBUTES>::is_open() const
{
    return m_socket.is_open();
}

CONNECTION_TEMPLATE_DECLARE
typename connection<CONNECTION_TEMPLATE_ATTRIBUTES>::socket_type connection<CONNECTION_TEMPLATE_ATTRIBUTES>::release()
{
    // making an rvalue reference since the copy is forbidden
    return std::move(m_socket);
}

} // namespace beast_router
