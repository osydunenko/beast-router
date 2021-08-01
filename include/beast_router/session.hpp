#pragma once

#include <any>
#include <regex>
#include <chrono>
#include <algorithm>
#include <type_traits>

#include <boost/beast/http/string_body.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "base/strand_stream.hpp"
#include "base/conn_queue.hpp"
#include "base/lockable.hpp"
#include "base/clb.hpp"
#include "common/connection.hpp"
#include "common/timer.hpp"
#include "router.hpp"

#define SESSION_TEMPLATE_ATTRIBUTES \
    Body, RequestParser, Buffer, Protocol, Socket

namespace beast_router {
namespace details {

template<class T>
struct is_chrono_duration: std::false_type {};

template<class Rep, class Period>
struct is_chrono_duration<std::chrono::duration<Rep, Period>>: std::true_type {};

template<class T>
constexpr bool is_chrono_duration_v = is_chrono_duration<T>::value;

} // namespace details

template<
    class Body = boost::beast::http::string_body,
    class RequestParser = boost::beast::http::request_parser<Body>,
    class Buffer = boost::beast::flat_buffer,
    class Protocol = boost::asio::ip::tcp,
    class Socket = boost::asio::basic_stream_socket<Protocol>
>
class session
{
    class impl;

public:
    template<class>
    class context;

    using self_type = session<SESSION_TEMPLATE_ATTRIBUTES>;

    using body_type = Body;

    using request_type = boost::beast::http::request<body_type>;

    template<class ResponseBody>
    using response_type = boost::beast::http::response<ResponseBody>;

    using request_parser_type = RequestParser;

    template<class ResponseBody>
    using response_serializer_type = boost::beast::http::response_serializer<ResponseBody>;

    using buffer_type = Buffer;

    using protocol_type = Protocol;

    using socket_type = Socket;

    using mutex_type = base::lockable::mutex_type;

    using impl_type = impl;

    using context_type = context<impl_type>;

    using connection_type = connection<socket_type, base::strand_stream::asio_type>;

    using timer_type = timer<base::strand_stream::asio_type, boost::asio::steady_timer>;

    using timer_duration_type = typename timer_type::duration_type;

    using on_error_type = std::function<void(boost::system::error_code, boost::string_view)>;

    using shutdown_type = typename socket_type::shutdown_type;

    using method_type = boost::beast::http::verb;

    using storage_type = base::clb::storage<self_type>;

    using resource_map_type = std::unordered_map<std::string, storage_type>; 

    using method_map_type = std::map<method_type, resource_map_type>;

    using router_type = router<self_type>;

    using conn_queue_type = base::conn_queue<impl_type>;

    template<class ...OnAction>
    static context_type recv(socket_type &&socket, const router_type &router, OnAction &&...onAction);

    template<class TimeDuration, class ...OnAction>
    static context_type recv(socket_type &&socket, const router_type &router, TimeDuration &&duration, OnAction &&...onAction);

private:

    template<class ...OnAction>
    static context_type init_context(socket_type &&socket, const router_type &router, OnAction &&...onAction);

    class impl: public base::strand_stream, public std::enable_shared_from_this<impl>
    {
        template<class>
        friend class context;

        template<class>
        friend class base::conn_queue;

        using method_const_map_pointer = typename router_type::method_const_map_pointer;

    public:
        using self_type = impl;

        explicit impl(socket_type &&socket, mutex_type &mutex, buffer_type &&buffer,
            method_const_map_pointer method_map,
            const on_error_type &on_error);

        self_type &recv();
        self_type &recv(timer_duration_type duration);

        template<class ResponseBody>
        self_type &send(const response_type<ResponseBody> &response);
        template<class ResponseBody>
        self_type &send(const response_type<ResponseBody> &response, timer_duration_type duration);

    private:
        void do_timer(timer_duration_type duraion);
        void on_timer(boost::system::error_code ec);
        void do_read();
        void on_read(boost::system::error_code ec, size_t bytes_transferred);
        void do_eof(shutdown_type type);
        void do_process_request();
        void provide(request_type &&request);
        template<class ResponseBody>
        void do_write(response_type<ResponseBody> &response);
        void on_write(boost::system::error_code ec, std::size_t bytes_transferred, bool close);

    private:
        connection_type m_connection;
        timer_type m_timer;
        mutex_type &m_mutex;
        buffer_type m_buffer;
        request_parser_type m_parser;
        method_const_map_pointer m_method_map; 
        on_error_type m_on_error;
        conn_queue_type m_queue;
        std::any m_serializer;
    };

public:
    template<class Impl>
    class context
    {
        static_assert(std::is_base_of_v<
            boost::asio::strand<boost::asio::system_timer::executor_type>, Impl>);

    public:
        context(Impl &impl);

        void recv();

        template<class TimeDuration>
        typename std::enable_if_t<details::is_chrono_duration_v<TimeDuration>> 
        recv(TimeDuration &&duration);

        template<class ResponseBody>
        void send(const response_type<ResponseBody> &response) const;

        template<class ResponseBody, class TimeDuration>
        typename std::enable_if_t<details::is_chrono_duration_v<TimeDuration>>
        send(const response_type<ResponseBody> &response, TimeDuration &&duration) const;

        bool is_open() const;
        
        template<class Type>
        Type &get_user_data() &;

        template<class Type>
        Type &&get_user_data() &&;

        template<class Type>
        void set_user_data(Type data);

    private:
        std::shared_ptr<Impl> m_impl;
        std::any m_user_data;
    };
};

} // namespace beast_router

#include "impl/session.hpp"
