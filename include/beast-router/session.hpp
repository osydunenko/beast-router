#pragma once

#include <any>
#include <regex>
#include <chrono>
#include <algorithm>
#include <type_traits>

#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/socket_base.hpp>

#include "base/strand_stream.hpp"
#include "base/lockable.hpp"
#include "base/cb.hpp"
#include "base/conn_queue.hpp"
#include "connection.hpp"
#include "timer.hpp"
#include "router.hpp"

#define SESSION_TEMPLATE_ATTRIBUTES \
    Body, RequestParser, Buffer, Protocol, Socket

#define SESSION_TEMPLATE_DECLARE \
    template<                 \
    class Body,               \
    class RequestParser,      \
    class Buffer,             \
    class Protocol,           \
    class Socket              \
    >

namespace beast_router {
namespace details {

template<class T>
struct is_chrono_duration: std::false_type {};

template<class Rep, class Period>
struct is_chrono_duration<std::chrono::duration<Rep, Period>>: std::true_type {};

template<class T>
constexpr bool is_chrono_duration_v = is_chrono_duration<T>::value;

} // namespace details


using std::declval;

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

    using storage_type = base::cb::storage<self_type>;

    using resource_map_type = std::unordered_map<std::string, storage_type>; 

    using method_map_type = std::map<method_type, resource_map_type>;

    using router_type = router<self_type>;

    using conn_queue_type = base::conn_queue<impl_type>;

    template<class ...OnAction>
    static context_type recv(socket_type &&socket, const router_type &router, OnAction &&...onAction);

    template<class TimeDuration, class ...OnAction>
    // TODO: move the templated TimeDuration check on ctx.recv()
    static typename std::enable_if_t<details::is_chrono_duration_v<TimeDuration>, context_type>
    recv(socket_type &&socket, const router_type &router, TimeDuration duration, OnAction &&...onAction);

private:

    template<class ...OnAction>
    static context_type init_context(socket_type &&socket, const router_type &router, OnAction &&...onAction);

    class impl: public base::strand_stream,
        public std::enable_shared_from_this<impl>
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
        self_type &send(response_type<ResponseBody> &response);

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
        void recv(timer_duration_type duration);

        template<class ResponseBody>
        void send(const response_type<ResponseBody> &response) const;

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

SESSION_TEMPLATE_DECLARE
template<class ...OnAction>
typename session<SESSION_TEMPLATE_ATTRIBUTES>::context_type
session<SESSION_TEMPLATE_ATTRIBUTES>::recv(socket_type &&socket, const router_type &router, OnAction &&...onAction)
{

    context_type ctx = init_context(std::move(socket), router, std::forward<OnAction>(onAction)...);
    ctx.recv();
    return ctx;
}

SESSION_TEMPLATE_DECLARE
template<class TimeDuration, class ...OnAction>
typename std::enable_if_t<details::is_chrono_duration_v<TimeDuration>, typename session<SESSION_TEMPLATE_ATTRIBUTES>::context_type>
session<SESSION_TEMPLATE_ATTRIBUTES>::recv(socket_type &&socket, const router_type &router, TimeDuration duration, OnAction &&...onAction)
{
    context_type ctx = init_context(std::move(socket), router, std::forward<OnAction>(onAction)...);
    ctx.recv(std::chrono::duration_cast<timer_duration_type>(std::move(duration)));
    return ctx;
}

SESSION_TEMPLATE_DECLARE
template<class ...OnAction>
typename session<SESSION_TEMPLATE_ATTRIBUTES>::context_type
session<SESSION_TEMPLATE_ATTRIBUTES>::init_context(socket_type &&socket, const router_type &router, OnAction &&...onAction)
{
    buffer_type buffer;
    return context_type{*std::make_shared<impl_type>(
        std::move(socket), 
        router.get_mutex(), 
        std::move(buffer),
        router.get_resource_map(),
        std::forward<OnAction>(onAction)...)
    };
}

SESSION_TEMPLATE_DECLARE
session<SESSION_TEMPLATE_ATTRIBUTES>::impl::impl(socket_type &&socket, mutex_type &mutex, buffer_type &&buffer, 
    method_const_map_pointer method_map,
    const on_error_type &on_error)
    : base::strand_stream{socket.get_executor()}
    , m_connection{std::move(socket), static_cast<base::strand_stream &>(*this)}
    , m_timer{static_cast<base::strand_stream &>(*this)}
    , m_mutex{mutex}
    , m_buffer{std::move(buffer)}
    , m_parser{}
    , m_method_map{method_map}
    , m_on_error{on_error}
    , m_queue{*this}
    , m_serializer{}
{
}

SESSION_TEMPLATE_DECLARE
typename session<SESSION_TEMPLATE_ATTRIBUTES>::impl::self_type &
session<SESSION_TEMPLATE_ATTRIBUTES>::impl::recv()
{
    do_read();
    return *this;
}

SESSION_TEMPLATE_DECLARE
typename session<SESSION_TEMPLATE_ATTRIBUTES>::impl::self_type &
session<SESSION_TEMPLATE_ATTRIBUTES>::impl::recv(timer_duration_type duration)
{
    do_timer(std::move(duration));
    do_read();
    return *this;
}

SESSION_TEMPLATE_DECLARE
template<class ResponseBody>
typename session<SESSION_TEMPLATE_ATTRIBUTES>::impl::self_type &
        session<SESSION_TEMPLATE_ATTRIBUTES>::impl::send(response_type<ResponseBody> &response)
{
    m_queue(response);
    return *this;
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::do_timer(timer_duration_type duration)
{
    m_timer.expires_from_now(std::move(duration));
    m_timer.async_wait(std::bind(&impl::on_timer, this->shared_from_this(), std::placeholders::_1));
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::on_timer(boost::system::error_code ec)
{
    if (ec && ec != boost::asio::error::operation_aborted) {
        m_on_error(ec, "async_timer/on_timer");
        return;
    }

    if (m_timer.expiry() <= std::chrono::steady_clock::now()) {
        do_eof(shutdown_type::shutdown_both);
    }
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::do_read()
{
    m_connection.async_read(m_buffer, m_parser,
        std::bind(&impl::on_read, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2)
    );
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::on_read(boost::system::error_code ec, size_t bytes_transferred)
{
    m_timer.cancel();
    static_cast<void>(bytes_transferred);
    if (ec == boost::beast::http::error::end_of_stream) {
        do_eof(shutdown_type::shutdown_both);
        return;
    }

    if (ec && m_on_error) {
        m_on_error(ec, "async_read/on_read");
    }

    do_process_request();
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::do_eof(shutdown_type type)
{
    if (!m_connection.is_open()) {
        return;
    }
    const auto ec = m_connection.shutdown(type);
    if (ec && m_on_error) {
        m_on_error(ec, "shutdown/eof");
    }
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::do_process_request()
{
    request_type request = m_parser.release();

    LOCKABLE_ENTER_TO_READ(m_mutex);  
    provide(std::move(request));
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::provide(request_type &&request)
{
    boost::string_view target = request.target(); 
    method_type method = request.method();

    const auto method_pos = m_method_map->find(method);
    if (method_pos != m_method_map->cend()) {
        auto &resource_map = method_pos->second;
        std::for_each(
            resource_map.begin(),
            resource_map.end(),
            [&](auto &val) {
                const std::regex re{val.first};
                const std::string target_string = target.to_string();
                std::smatch base_match;
                if (std::regex_match(target_string, base_match, re)) {
                    storage_type &storage = const_cast<storage_type &>(val.second);
                    context_type ctx{*this};
                    storage.begin_execute(request, std::move(ctx), std::move(base_match));
                }
            }
        );
    }
}

SESSION_TEMPLATE_DECLARE
template<class ResponseBody>
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::do_write(response_type<ResponseBody> &response)
{
    using serializer_type = response_serializer_type<ResponseBody>;

    m_serializer = serializer_type(response);
    m_connection.async_write(
        std::any_cast<serializer_type &>(m_serializer),
        std::bind(
            &impl::on_write, 
            this->shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2,
            response.need_eof()
        )
    );
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::on_write(boost::system::error_code ec, std::size_t bytes_transferred, bool close)
{
    static_cast<void>(bytes_transferred);

    if (ec && m_on_error) {
        m_on_error(ec, "async_read/on_write");
        return;
    }

    if (close) {
        do_eof(shutdown_type::shutdown_both);
        return;
    }

    m_queue.on_write();
}

SESSION_TEMPLATE_DECLARE
template<class Impl>
session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::context(Impl &impl)
    : m_impl{impl.shared_from_this()}
{
    assert(m_impl != nullptr);
}

SESSION_TEMPLATE_DECLARE
template<class Impl>
void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::recv()
{
    assert(m_impl != nullptr);
    boost::asio::dispatch(
        static_cast<base::strand_stream>(*m_impl),
        std::bind(
            static_cast<Impl &(Impl::*)()>(&Impl::recv),
            m_impl->shared_from_this()
        )
    );
}

SESSION_TEMPLATE_DECLARE
template<class Impl>
void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::recv(timer_duration_type duration)
{
    assert(m_impl != nullptr);
    boost::asio::dispatch(
        static_cast<base::strand_stream>(*m_impl),
        std::bind(
            static_cast<Impl &(Impl::*)(timer_duration_type)>(&Impl::recv),
            m_impl->shared_from_this(),
            std::move(duration)
        )
    );
}

SESSION_TEMPLATE_DECLARE
template<class Impl>
template<class ResponseBody>
void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::send(const response_type<ResponseBody> &response) const
{
    assert(m_impl != nullptr);
    boost::asio::dispatch(
        static_cast<base::strand_stream &>(*m_impl),
        std::bind(
            static_cast<Impl &(Impl::*)(response_type<ResponseBody> &)>(&Impl::send),
            m_impl->shared_from_this(),
            response
        )
    );
}

SESSION_TEMPLATE_DECLARE
template<class Impl>
bool session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::is_open() const
{
    assert(m_impl != nullptr);
    return m_impl->m_connection.is_open();
}

SESSION_TEMPLATE_DECLARE
template<class Impl>
template<class Type>
Type &session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::get_user_data() &
{
    return std::any_cast<Type &>(m_user_data);
}

SESSION_TEMPLATE_DECLARE
template<class Impl>
template<class Type>
Type &&session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::get_user_data() &&
{
    return std::any_cast<Type &&>(std::move(m_user_data));
}

SESSION_TEMPLATE_DECLARE
template<class Impl>
template<class Type>
void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::set_user_data(Type data)
{
    m_user_data = std::make_any<Type>(std::move(data));
}

} // namespace beast_router
