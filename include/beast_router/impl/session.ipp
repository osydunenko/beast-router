#pragma once

namespace beast_router {

#define SESSION_TEMPLATE_DECLARE \
    template<                 \
    class Body,               \
    class RequestParser,      \
    class Buffer,             \
    class Protocol,           \
    class Socket              \
    >

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
typename session<SESSION_TEMPLATE_ATTRIBUTES>::context_type
session<SESSION_TEMPLATE_ATTRIBUTES>::recv(socket_type &&socket, const router_type &router, TimeDuration &&duration, OnAction &&...onAction)
{
    context_type ctx = init_context(std::move(socket), router, std::forward<OnAction>(onAction)...);
    ctx.recv(std::forward<TimeDuration>(duration));
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
template<class Response>
typename session<SESSION_TEMPLATE_ATTRIBUTES>::impl::self_type &
session<SESSION_TEMPLATE_ATTRIBUTES>::impl::send(Response &&response, timer_duration_type duration)
{
    do_timer(std::move(duration));
    m_queue(std::forward<Response>(response));
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

    if (m_timer.expiry() <= timer_type::clock_type::now()) {
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
    boost::ignore_unused(bytes_transferred);

    m_timer.cancel();

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
    
    auto make_context = [this]() mutable
        -> context_type {
        return context_type{*this};
    };

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
                    if (const_cast<storage_type &>(val.second)
                            .begin_execute(request, make_context(), std::move(base_match))) {
                        return;
                    }
                }
            }
        );
    }

    if (const auto not_found = m_method_map->find(method_type::unknown);
        not_found != m_method_map->cend()) {
        auto &resource_map = not_found->second;
        if (const auto storage = resource_map.find("");
            storage != resource_map.cend()) {
            const_cast<storage_type &>(storage->second)
                .begin_execute(request, make_context(), {});
        }
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
    boost::ignore_unused(bytes_transferred);

    m_timer.cancel();

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
template<class TimeDuration>
typename std::enable_if_t<utility::is_chrono_duration_v<TimeDuration>>
session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::recv(TimeDuration &&duration)
{
    assert(m_impl != nullptr);
    boost::asio::dispatch(
        static_cast<base::strand_stream>(*m_impl),
        std::bind(
            static_cast<Impl &(Impl::*)(timer_duration_type)>(&Impl::recv),
            m_impl->shared_from_this(),
            std::chrono::duration_cast<timer_duration_type>(std::forward<TimeDuration>(duration))
        )
    );
}

SESSION_TEMPLATE_DECLARE
template<class Impl>
template<class Response>
void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::send(Response &&response) const
{
    do_send(std::forward<Response>(response), timer_duration_type::max());
}

SESSION_TEMPLATE_DECLARE
template<class Impl>
template<class Response, class TimeDuration>
void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::send(Response &&response, TimeDuration &&duration) const
{
    do_send(std::forward<Response>(response), std::forward<TimeDuration>(duration));
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

SESSION_TEMPLATE_DECLARE
template<class Impl>
template<class Response, class TimeDuration>
void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::do_send(Response &&response, TimeDuration &&duration) const
{
    static_assert(utility::is_chrono_duration_v<TimeDuration>,
        "TimeDuration requirements are not met");

    assert(m_impl != nullptr);

    auto callback = [
        rsp = std::forward<Response>(response), 
        dur = std::forward<TimeDuration>(duration),
        impl = m_impl->shared_from_this()
    ]() mutable -> Impl &
    {
        return impl->send(
            std::forward<Response>(rsp), 
            std::forward<TimeDuration>(dur));
    };

    boost::asio::dispatch(
        static_cast<base::strand_stream &>(*m_impl),
        std::move(callback)
    );
}

} // namespace beast_router
