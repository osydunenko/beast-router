#pragma once

#include <boost/asio/bind_executor.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/read.hpp>

#define CONNECTION_TEMPLATE_DECLARE \
    template<                       \
    class Socket,                   \
    class CompletionExecutor        \
    >

#define CONNECTION_TEMPLATE_ATTRIBUTES \
    Socket, CompletionExecutor

namespace beast_router {

template<class Socket, class CompletionExecutor>
class connection
{
public:
    using self_type = connection<CONNECTION_TEMPLATE_ATTRIBUTES>;

    using socket_type = Socket;

    using shutdown_type = typename socket_type::shutdown_type;

    explicit connection(Socket &&socket, const CompletionExecutor &executor);
    connection(const self_type &) = delete;
    self_type &operator=(const self_type &) = delete;
    connection(self_type &&) = delete;
    self_type &operator=(self_type &&) = delete;

    template <class Function, class Serializer>
    void async_write(Serializer &serializer, Function &&func);

    template <class Function, class Buffer, class Parser>
    void async_read(Buffer &buffer, Parser &parser, Function &&func);

    boost::beast::error_code shutdown(shutdown_type type);
    boost::beast::error_code close();

    bool is_open() const;

private:
    Socket m_socket;
    const CompletionExecutor &m_complition_executor;
};

CONNECTION_TEMPLATE_DECLARE
connection<CONNECTION_TEMPLATE_ATTRIBUTES>::connection(Socket &&socket, const CompletionExecutor &executor)
    : m_socket{std::move(socket)}
    , m_complition_executor{executor}
{
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
