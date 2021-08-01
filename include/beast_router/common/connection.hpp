#pragma once

#include <boost/asio/bind_executor.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/read.hpp>

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
    ~connection();

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

} // namespace beast_router

#include "impl/connection.hpp"
