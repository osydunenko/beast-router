#pragma once

ROUTER_NAMESPACE_BEGIN()

template <bool IsRequest, class Body>
struct serializer;

template <class Body>
struct serializer<true, Body> {
    using type = boost::beast::http::request_serializer<Body>;
};

template <class Body>
struct serializer<false, Body> {
    using type = boost::beast::http::response_serializer<Body>;
};

#define SESSION_TEMPLATE_DECLARE                                        \
    template <bool IsRequest, class Body, class Buffer, class Protocol, \
        class Socket, class Connection>
#define SESSION_TEMPLATE_ATTRIBUTES \
    IsRequest, Body, Buffer, Protocol, Socket, Connection

SESSION_TEMPLATE_DECLARE
template <class... OnAction>
typename session<SESSION_TEMPLATE_ATTRIBUTES>::context_type
session<SESSION_TEMPLATE_ATTRIBUTES>::init_context(socket_type&& socket,
    const router_type& router,
    OnAction&&... on_action)
{
    buffer_type buffer;
    return context_type {
        *std::make_shared<impl_type>(std::move(socket), std::move(buffer),
            router, std::forward<OnAction>(on_action)...)
    };
}

#if defined(LINK_SSL)
SESSION_TEMPLATE_DECLARE
template <class... OnAction>
typename session<SESSION_TEMPLATE_ATTRIBUTES>::context_type
session<SESSION_TEMPLATE_ATTRIBUTES>::init_context(boost::asio::ssl::context& ssl_ctx,
    socket_type&& socket,
    const router_type& router,
    OnAction&&... on_action)
{
    buffer_type buffer;
    return context_type {
        *std::make_shared<impl_type>(ssl_ctx, std::move(socket), std::move(buffer),
            router, std::forward<OnAction>(on_action)...)
    };
}
#endif

SESSION_TEMPLATE_DECLARE
session<SESSION_TEMPLATE_ATTRIBUTES>::impl::impl(socket_type&& socket,
    buffer_type&& buffer,
    const router_type& router,
    const on_error_type& on_error)
    : base::strand_stream { socket.get_executor() }
    , m_connection { std::move(socket), static_cast<base::strand_stream&>(*this) }
    , m_timer { static_cast<base::strand_stream&>(*this) }
    , m_buffer { std::move(buffer) }
    , m_on_error { on_error }
    , m_queue { *this }
    , m_serializer {}
    , m_parser {}
    , m_dispatcher { router }
{
}

#if defined(LINK_SSL)
SESSION_TEMPLATE_DECLARE
session<SESSION_TEMPLATE_ATTRIBUTES>::impl::impl(boost::asio::ssl::context& ssl_ctx, socket_type&& socket,
    buffer_type&& buffer,
    const router_type& router,
    const on_error_type& on_error)
    : base::strand_stream { socket.get_executor() }
    , m_connection { std::move(socket), ssl_ctx, static_cast<base::strand_stream&>(*this) }
    , m_timer { static_cast<base::strand_stream&>(*this) }
    , m_buffer { std::move(buffer) }
    , m_on_error { on_error }
    , m_queue { *this }
    , m_serializer {}
    , m_parser {}
    , m_dispatcher { router }
{
}
#endif

SESSION_TEMPLATE_DECLARE
typename session<SESSION_TEMPLATE_ATTRIBUTES>::impl::self_type&
session<SESSION_TEMPLATE_ATTRIBUTES>::impl::recv()
{
    do_read();
    return *this;
}

SESSION_TEMPLATE_DECLARE
typename session<SESSION_TEMPLATE_ATTRIBUTES>::impl::self_type&
session<SESSION_TEMPLATE_ATTRIBUTES>::impl::recv(timer_duration_type duration)
{
    do_timer(std::move(duration));
    do_read();
    return *this;
}

SESSION_TEMPLATE_DECLARE
template <class Message>
typename session<SESSION_TEMPLATE_ATTRIBUTES>::impl::self_type&
session<SESSION_TEMPLATE_ATTRIBUTES>::impl::send(Message&& message,
    timer_duration_type duration)
{
    do_timer(std::move(duration));
    m_queue(std::forward<Message>(message));
    return *this;
}

SESSION_TEMPLATE_DECLARE
template <class Message>
typename session<SESSION_TEMPLATE_ATTRIBUTES>::impl::self_type&
session<SESSION_TEMPLATE_ATTRIBUTES>::impl::send(Message&& message)
{
    m_queue(std::forward<Message>(message));
    return *this;
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::do_timer(
    timer_duration_type duration)
{
    m_timer.expires_from_now(std::move(duration));
    m_timer.async_wait(std::bind(&impl::on_timer, this->shared_from_this(),
        std::placeholders::_1));
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::on_timer(
    boost::system::error_code ec)
{
    if (ec && ec != boost::asio::error::operation_aborted) {
        if (m_on_error) {
            m_on_error(ec, "async_timer/on_timer");
        }
        return;
    }

    if (m_timer.expiry() <= timer_type::clock_type::now()) {
        if (m_on_error) {
            auto ec = make_error_code(boost::asio::error::timed_out);
            m_on_error(ec, "async_timer/on_timer");
        }
        do_eof(shutdown_type::shutdown_both);
    }
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::do_read()
{
    m_connection.async_read(
        m_buffer, m_parser,
        std::bind(&impl::on_read, this->shared_from_this(), std::placeholders::_1,
            std::placeholders::_2));
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::on_read(
    boost::system::error_code ec, [[maybe_unused]] size_t bytes_transferred)
{
    m_timer.cancel();

    if (ec == boost::beast::http::error::end_of_stream) {
        do_eof(shutdown_type::shutdown_both);
        return;
    }

    if (ec && m_on_error) {
        m_on_error(ec, "async_read/on_read");
    }

    do_process_request(m_parser.release());
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
template <class Message>
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::do_process_request(
    Message&& message)
{
    m_dispatcher.do_process_request(std::forward<Message>(message), *this);
}

SESSION_TEMPLATE_DECLARE
template <bool IsMessageRequest, class MessageBody, class Fields>
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::do_write(
    boost::beast::http::message<IsMessageRequest, MessageBody, Fields>& message)
{
    using serializer_type =
        typename serializer<IsMessageRequest, MessageBody>::type;

    m_serializer = std::make_any<serializer_type>(message);

    m_connection.async_write(
        std::any_cast<serializer_type&>(m_serializer),
        std::bind(&impl::on_write, this->shared_from_this(),
            std::placeholders::_1, std::placeholders::_2,
            message.need_eof()));
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::on_write(
    boost::system::error_code ec,
    [[maybe_unused]] std::size_t bytes_transferred, bool close)
{
    m_timer.cancel();

    if (ec == boost::beast::http::error::end_of_stream) {
        do_eof(shutdown_type::shutdown_both);
        return;
    }

    if (ec && m_on_error) {
        m_on_error(ec, "async_write/on_write");
        return;
    }

    if (close) {
        do_eof(shutdown_type::shutdown_both);
        return;
    }

    m_queue.on_write();
}

#if defined(LINK_SSL)
SESSION_TEMPLATE_DECLARE
template <class Func>
typename session<SESSION_TEMPLATE_ATTRIBUTES>::impl::self_type&
session<SESSION_TEMPLATE_ATTRIBUTES>::impl::do_handshake(Func&& func)
{
    auto clb = [impl = this->shared_from_this(), f = std::forward<Func>(func)](const boost::system::error_code& error) {
        BOOST_ASSERT(impl);
        impl->on_handshake(error);
        if (not error) {
            context_type ctx { *impl };
            f(ctx);
        }
    };

    if constexpr (is_request::value) {
        m_connection.async_handshake(boost::asio::ssl::stream_base::server, std::move(clb));
    } else {
        m_connection.async_handshake(boost::asio::ssl::stream_base::client, std::move(clb));
    }
    return *this;
}

SESSION_TEMPLATE_DECLARE
template <class Func>
typename session<SESSION_TEMPLATE_ATTRIBUTES>::impl::self_type&
session<SESSION_TEMPLATE_ATTRIBUTES>::impl::do_handshake(Func&& func, timer_duration_type duration)
{
    m_timer.expires_from_now(std::move(duration));
    return do_handshake(std::forward<Func>(func));
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::on_handshake(boost::system::error_code ec)
{
    m_timer.cancel();

    if (ec == boost::beast::http::error::end_of_stream) {
        do_eof(shutdown_type::shutdown_both);
        return;
    }

    if (ec && m_on_error) {
        m_on_error(ec, "async_handshake/on_handshake");
        return;
    }
}
#endif

SESSION_TEMPLATE_DECLARE
template <class Impl>
session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::context(Impl& impl)
    : m_impl { impl.shared_from_this() }
{
    BOOST_ASSERT(m_impl != nullptr);
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
ROUTER_DECL void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::recv()
{
    BOOST_ASSERT(m_impl != nullptr);
    boost::asio::dispatch(static_cast<base::strand_stream>(*m_impl),
        std::bind(static_cast<Impl& (Impl::*)()>(&Impl::recv),
            m_impl->shared_from_this()));
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
template <class TimeDuration>
ROUTER_DECL void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::recv(
    TimeDuration&& duration)
{
    /*static_assert(utility::is_chrono_duration_v<TimeDuration>,
        "TimeDuration requirements are not met");*/

    BOOST_ASSERT(m_impl != nullptr);
    boost::asio::dispatch(
        static_cast<base::strand_stream>(*m_impl),
        std::bind(static_cast<Impl& (Impl::*)(timer_duration_type)>(&Impl::recv),
            m_impl->shared_from_this(),
            std::chrono::duration_cast<timer_duration_type>(
                std::forward<TimeDuration>(duration))));
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
template <class Message>
ROUTER_DECL void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::send(
    Message&& message) const
{
    BOOST_ASSERT(m_impl != nullptr);
    boost::asio::dispatch(static_cast<base::strand_stream>(*m_impl),
        [impl = m_impl->shared_from_this(), msg = std::move(message)]() mutable {
            impl->send(std::move(msg));
        });
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
template <class Message, class TimeDuration>
ROUTER_DECL void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::send(
    Message&& message, TimeDuration&& duration) const
{
    static_assert(utility::is_chrono_duration_v<TimeDuration>,
        "TimeDuration requirements are not met");

    BOOST_ASSERT(m_impl != nullptr);
    boost::asio::dispatch(static_cast<base::strand_stream>(*m_impl),
        [impl = m_impl->shared_from_this(), msg = std::move(message), dur = std::move(duration)]() mutable {
            impl->send(std::move(msg), std::move(dur));
        });
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
template <class Func>
ROUTER_DECL void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::handshake(Func&& func) const
{
    static_assert(std::is_invocable_v<Func, context_type&>,
        "Func requirements are not met");

    BOOST_ASSERT(m_impl != nullptr);
    boost::asio::dispatch(static_cast<base::strand_stream>(*m_impl),
        [impl = m_impl->shared_from_this(), f = std::move(func)]() mutable {
            impl->do_handshake(std::move(f));
        });
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
template <class Func, class TimeDuration>
ROUTER_DECL void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::handshake(Func&& func, TimeDuration&& duration) const
{
    static_assert(utility::is_chrono_duration_v<TimeDuration>,
        "TimeDuration requirements are not met");
    static_assert(std::is_invocable_v<Func, context_type&>,
        "Func requirements are not met");

    BOOST_ASSERT(m_impl != nullptr);
    boost::asio::dispatch(static_cast<base::strand_stream>(*m_impl),
        [impl = m_impl->shared_from_this(), f = std::move(func), dur = std::move(duration)]() mutable {
            impl->do_handshake(std::move(f), std::move(dur));
        });
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
ROUTER_DECL bool session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::is_open() const
{
    BOOST_ASSERT(m_impl != nullptr);
    return m_impl->m_connection.is_open();
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
template <class Type>
ROUTER_DECL Type& session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::get_user_data() &
{
    return std::any_cast<Type&>(m_user_data);
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
template <class Type>
ROUTER_DECL Type&& session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::get_user_data() &&
{
    return std::any_cast<Type&&>(std::move(m_user_data));
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
template <class Type>
ROUTER_DECL void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::set_user_data(
    Type&& data)
{
    m_user_data = std::make_any<Type>(std::forward<Type>(data));
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
ROUTER_DECL typename session<SESSION_TEMPLATE_ATTRIBUTES>::connection_type::stream_type&
session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::get_stream()
{
    return m_impl->m_connection.stream();
}

ROUTER_NAMESPACE_END()
