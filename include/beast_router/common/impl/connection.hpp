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
    static_assert(std::is_invocable_v<Function, boost::system::error_code, size_t>);
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
    static_assert(std::is_invocable_v<Function, boost::system::error_code, size_t>);
    boost::beast::http::async_read(m_socket, buffer, parser,
        boost::asio::bind_executor(
            m_complition_executor, std::forward<Function>(func)
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

} // namespace beast_router
