#pragma once

#include <regex>
#include <algorithm>
#include <queue>
#include <any>
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
#include "connection.hpp"
#include "router.hpp"

#define SESSION_TEMPLATE_ATTRIBUTES \
    Body, RequestParser, ResponseSerializer, Buffer, Protocol, Socket

#define SESSION_TEMPLATE_DECLARE \
    template<                 \
    class Body,               \
    class RequestParser,      \
    class ResponseSerializer, \
    class Buffer,             \
    class Protocol,           \
    class Socket              \
    >

namespace server {

template<
    class Body = boost::beast::http::string_body,
    class RequestParser = boost::beast::http::request_parser<Body>,
    class ResponseSerializer = boost::beast::http::response_serializer<Body>,
    class Buffer = boost::beast::flat_buffer,
    class Protocol = boost::asio::ip::tcp,
    class Socket = boost::asio::basic_stream_socket<Protocol>
>
class session
{
    class flesh;
public:
    template<class>
    class context;

    using self_type = session<SESSION_TEMPLATE_ATTRIBUTES>;

    using body_type = Body;

    using request_type = boost::beast::http::request<body_type>;

    template<class ResponseBody>
    using response_type = boost::beast::http::response<ResponseBody>;

    using request_parser_type = RequestParser;

    using response_serializer_type = ResponseSerializer;

    using buffer_type = Buffer;

    using protocol_type = Protocol;

    using socket_type = Socket;

    using mutex_type = base::lockable::mutex_type;

    using flesh_type = flesh;

    using context_type = context<flesh_type>;

    using connection_type = connection<socket_type, base::strand_stream::asio_type>;

    using on_error_type = std::function<void(boost::system::error_code, boost::string_view)>;

    using shutdown_type = typename socket_type::shutdown_type;

    using method_type = boost::beast::http::verb;

    using storage_type = base::cb::storage<self_type>;

    using resource_map_type = std::unordered_map<std::string, storage_type>; 

    using method_map_type = std::map<method_type, resource_map_type>;

    using router_type = router<self_type>;

    template<class ...OnAction>
    static context_type recv(socket_type &&socket, const router_type &router, const OnAction &...onAction);

private:

    class flesh: public base::strand_stream,
        public std::enable_shared_from_this<flesh>
    {
        template<class>
        friend class context;

        using method_const_map_pointer = typename router_type::method_const_map_pointer;

        class queue
        {
            struct wrk
            {
                virtual ~wrk() = default;
                virtual void operator()() = 0;
            };

        public:
            template<class Response>
            void operator()(Response &response)
            {
                using response_type = std::decay_t<Response>; 

                struct wrk_impl: wrk
                {
                    wrk_impl(flesh &fl, Response &&response)
                        : m_flesh{fl}
                        , m_response{std::move(response)}
                    {
                    }

                    void operator()() override
                    {
                        m_flesh.do_write(m_response);
                    }

                    flesh &m_flesh;
                    response_type m_response;
                };

                m_items.push(
                    std::make_unique<wrk_impl>(
                        m_flesh, 
                        std::move(response)
                    )
                );

                if (m_items.size() == 1) {
                    (*m_items.front())();
                }
            }

            void on_write()
            {
                m_items.pop();
                if (m_items.size() > 0) {
                    (*m_items.front())();
                }
            }

            explicit queue(flesh &fl)
                : m_flesh{fl}
            {
            }
            
        private:
            flesh &m_flesh;
            std::queue<std::unique_ptr<wrk>> m_items{};
        };

    public:
        using self_type = flesh;

        explicit flesh(socket_type &&socket, mutex_type &mutex, buffer_type &&buffer, 
            method_const_map_pointer method_map,
            const on_error_type &on_error);

        self_type &recv();

        template<class ResponseBody>
        self_type &send(response_type<ResponseBody> &response);

    private:
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
        mutex_type &m_mutex;
        buffer_type m_buffer;
        request_parser_type m_parser;
        method_const_map_pointer m_method_map; 
        on_error_type m_on_error;
        queue m_queue;
        std::optional<response_serializer_type> m_serializer;
    };

public:
    template<class Flesh>
    class context
    {
    public:
        context(Flesh &flesh);

        void recv() const;

        template<class ResponseBody>
        void send(const response_type<ResponseBody> &response) const;

        bool is_open() const 
        {
            return m_flesh->m_connection.is_open();
        }

        template<class Type>
        std::enable_if_t<std::is_reference_v<Type>, std::pair<bool, Type>> get_user_data();

        template<class Type>
        std::enable_if_t<!std::is_reference_v<Type>, std::pair<bool, Type>> get_user_data();

        template<class Type>
        void set_user_data(Type &&data);

    private:
        std::shared_ptr<Flesh> m_flesh;
        std::any m_user_data;
    };
};

using default_session = session<>;

SESSION_TEMPLATE_DECLARE
template<class ...OnAction>
typename session<SESSION_TEMPLATE_ATTRIBUTES>::context_type
session<SESSION_TEMPLATE_ATTRIBUTES>::recv(socket_type &&socket, const router_type &router, const OnAction &...onAction)
{
    buffer_type buffer;
    context_type ctx(*std::make_shared<flesh_type>(
                std::move(socket), 
                router.get_mutex(), 
                std::move(buffer),
                router.get_resource_map(),
                onAction...)
    );
    ctx.recv();
    return ctx;
}

SESSION_TEMPLATE_DECLARE
session<SESSION_TEMPLATE_ATTRIBUTES>::flesh::flesh(socket_type &&socket, mutex_type &mutex, buffer_type &&buffer, 
    method_const_map_pointer method_map,
    const on_error_type &on_error)
    : base::strand_stream{socket.get_executor()}
    , m_connection{std::move(socket), static_cast<base::strand_stream &>(*this)}
    , m_mutex{mutex}
    , m_buffer{std::move(buffer)}
    , m_parser{}
    , m_method_map{method_map}
    , m_on_error{on_error}
    , m_queue{*this}
{
}

SESSION_TEMPLATE_DECLARE
typename session<SESSION_TEMPLATE_ATTRIBUTES>::flesh::self_type &session<SESSION_TEMPLATE_ATTRIBUTES>::flesh::recv()
{
    do_read();
    return *this;
}

SESSION_TEMPLATE_DECLARE
template<class ResponseBody>
typename session<SESSION_TEMPLATE_ATTRIBUTES>::flesh::self_type &session<SESSION_TEMPLATE_ATTRIBUTES>::flesh::send(response_type<ResponseBody> &response)
{
    m_queue(response);
    return *this;
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::flesh::on_read(boost::system::error_code ec, size_t bytes_transferred)
{
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
void session<SESSION_TEMPLATE_ATTRIBUTES>::flesh::do_read()
{
    m_connection.async_read(m_buffer, m_parser,
        std::bind(&flesh::on_read, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2)
    );
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::flesh::do_eof(shutdown_type type)
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
void session<SESSION_TEMPLATE_ATTRIBUTES>::flesh::do_process_request()
{
    request_type request = m_parser.release();

    LOCKABLE_ENTER_TO_READ(m_mutex);  
    provide(std::move(request));
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::flesh::provide(request_type &&request)
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
void session<SESSION_TEMPLATE_ATTRIBUTES>::flesh::do_write(response_type<ResponseBody> &response)
{
    m_serializer.emplace(response);
    m_connection.async_write(
        *m_serializer,
        std::bind(
            &flesh::on_write, 
            this->shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2,
            response.need_eof()
        )
    );
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::flesh::on_write(boost::system::error_code ec, std::size_t bytes_transferred, bool close)
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
template<class Flesh>
session<SESSION_TEMPLATE_ATTRIBUTES>::context<Flesh>::context(Flesh &flesh)
    : m_flesh{flesh.shared_from_this()}
{
    assert(m_flesh != nullptr);
}

SESSION_TEMPLATE_DECLARE
template<class Flesh>
void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Flesh>::recv() const
{
    assert(m_flesh != nullptr);
    boost::asio::dispatch(
        static_cast<base::strand_stream>(*m_flesh),
        std::bind(
            static_cast<Flesh &(Flesh::*)()>(&Flesh::recv),
            m_flesh->shared_from_this()
        )
    );
}

SESSION_TEMPLATE_DECLARE
template<class Flesh>
template<class ResponseBody>
void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Flesh>::send(const response_type<ResponseBody> &response) const
{
    assert(m_flesh != nullptr);
    boost::asio::dispatch(
        static_cast<base::strand_stream &>(*m_flesh),
        std::bind(
            static_cast<Flesh &(Flesh::*)(response_type<ResponseBody> &)>(&Flesh::send),
            m_flesh->shared_from_this(),
            response
        )
    );
}

SESSION_TEMPLATE_DECLARE
template<class Flesh>
template<class Type>
std::enable_if_t<std::is_reference_v<Type>, std::pair<bool, Type>>
session<SESSION_TEMPLATE_ATTRIBUTES>::context<Flesh>::get_user_data()
{
    using raw_type = std::decay_t<Type>;
    static_assert(std::is_default_constructible_v<raw_type>);
    static raw_type ret{};

    if (!m_user_data.has_value()) {
        return {false, ret};
    }
    
    try {
        return {true, std::any_cast<Type&>(m_user_data)};
    } catch(const std::bad_any_cast &) { 
        return {false, ret};
    }
}

SESSION_TEMPLATE_DECLARE
template<class Flesh>
template<class Type>
std::enable_if_t<!std::is_reference_v<Type>, std::pair<bool, Type>>
session<SESSION_TEMPLATE_ATTRIBUTES>::context<Flesh>::get_user_data()
{
    using raw_type = std::decay_t<Type>;
    static_assert(std::is_default_constructible_v<raw_type>);

    if (!m_user_data.has_value()) {
        return {false, Type{}};
    }
    
    try {
        return {true, std::any_cast<Type>(m_user_data)};
    } catch (const std::bad_any_cast &) { 
        return {false, Type{}};
    }
}

SESSION_TEMPLATE_DECLARE
template<class Flesh>
template<class Type>
void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Flesh>::set_user_data(Type &&data)
{
    m_user_data = std::make_any<Type>(std::move(data));
}

} // namespace server
