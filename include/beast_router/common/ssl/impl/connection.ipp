#pragma once

#define SSL_CONNECTION_TEMPLATE_DECLARE \
    template <class Stream, class CompletionExecutor>

ROUTER_SSL_NAMESPACE_BEGIN()

SSL_CONNECTION_TEMPLATE_DECLARE
connection<SSL_CONNECTION_TEMPLATE_ATTRIBUTES>::connection(
    Stream&& stream, boost::asio::ssl::context& ctx, const CompletionExecutor& executor)
    : base::connection<connection<Stream, CompletionExecutor>,
        CompletionExecutor> { executor }
    , m_stream { std::move(stream), ctx }
{
}

SSL_CONNECTION_TEMPLATE_DECLARE
connection<SSL_CONNECTION_TEMPLATE_ATTRIBUTES>::~connection()
{
    if (is_open()) {
        shutdown(shutdown_type::shutdown_both);
    }
}

SSL_CONNECTION_TEMPLATE_DECLARE
template <class Function>
void connection<SSL_CONNECTION_TEMPLATE_ATTRIBUTES>::async_handshake(
    connection<SSL_CONNECTION_TEMPLATE_ATTRIBUTES>::handshake_type type,
    Function&& func)
{
    static_assert(
        utility::details::is_all_true<
            std::is_invocable_v<Function, boost::system::error_code>>::value,
        "connection::async_handshake requirements are not met");

    m_stream.async_handshake(
        type, boost::asio::bind_executor(this->m_completion_executor, std::forward<Function>(func)));
}

SSL_CONNECTION_TEMPLATE_DECLARE
boost::beast::error_code
connection<SSL_CONNECTION_TEMPLATE_ATTRIBUTES>::shutdown(shutdown_type type)
{
    auto ec = boost::system::error_code {};
    m_stream.next_layer().shutdown(type, ec);

    return ec;
}

SSL_CONNECTION_TEMPLATE_DECLARE
boost::beast::error_code
connection<SSL_CONNECTION_TEMPLATE_ATTRIBUTES>::close()
{
    auto ec = boost::system::error_code {};
    m_stream.next_layer().close(ec);

    return ec;
}

SSL_CONNECTION_TEMPLATE_DECLARE
bool connection<SSL_CONNECTION_TEMPLATE_ATTRIBUTES>::is_open() const
{
    return m_stream.is_open();
}

SSL_CONNECTION_TEMPLATE_DECLARE
typename connection<SSL_CONNECTION_TEMPLATE_ATTRIBUTES>::stream_type
connection<SSL_CONNECTION_TEMPLATE_ATTRIBUTES>::release()
{
    // making an rvalue reference since the copy is forbidden
    return std::move(m_stream);
}

SSL_CONNECTION_TEMPLATE_DECLARE
typename connection<SSL_CONNECTION_TEMPLATE_ATTRIBUTES>::stream_type&
connection<SSL_CONNECTION_TEMPLATE_ATTRIBUTES>::stream()
{
    return m_stream;
}

ROUTER_SSL_NAMESPACE_END()