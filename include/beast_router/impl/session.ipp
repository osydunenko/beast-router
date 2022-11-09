#pragma once

namespace beast_router {
namespace details {

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

}  // namespace details

#define SESSION_TEMPLATE_DECLARE                                      \
  template <bool IsRequest, class Body, class Buffer, class Protocol, \
            class Socket>

SESSION_TEMPLATE_DECLARE
template <class... OnAction>
typename session<SESSION_TEMPLATE_ATTRIBUTES>::context_type
session<SESSION_TEMPLATE_ATTRIBUTES>::recv(socket_type &&socket,
                                           const router_type &router,
                                           OnAction &&...on_action) {
  context_type ctx = init_context(std::move(socket), router,
                                  std::forward<OnAction>(on_action)...);
  ctx.recv();
  return ctx;
}

SESSION_TEMPLATE_DECLARE
template <class TimeDuration, class... OnAction>
typename session<SESSION_TEMPLATE_ATTRIBUTES>::context_type
session<SESSION_TEMPLATE_ATTRIBUTES>::recv(socket_type &&socket,
                                           const router_type &router,
                                           TimeDuration &&duration,
                                           OnAction &&...on_action) {
  context_type ctx = init_context(std::move(socket), router,
                                  std::forward<OnAction>(on_action)...);
  ctx.recv(std::forward<TimeDuration>(duration));
  return ctx;
}

SESSION_TEMPLATE_DECLARE
template <class Request, class... OnAction>
typename session<SESSION_TEMPLATE_ATTRIBUTES>::context_type
session<SESSION_TEMPLATE_ATTRIBUTES>::send(socket_type &&socket,
                                           Request &&request,
                                           const router_type &router,
                                           OnAction &&...on_action) {
  static_assert(!IsRequest, "session::send requirements are not met");
  context_type ctx = init_context(std::move(socket), router,
                                  std::forward<OnAction>(on_action)...);
  ctx.send(std::forward<Request>(request));
  return ctx;
}

SESSION_TEMPLATE_DECLARE
template <class... OnAction>
typename session<SESSION_TEMPLATE_ATTRIBUTES>::context_type
session<SESSION_TEMPLATE_ATTRIBUTES>::init_context(socket_type &&socket,
                                                   const router_type &router,
                                                   OnAction &&...on_action) {
  buffer_type buffer;
  return context_type{
      *std::make_shared<impl_type>(std::move(socket), std::move(buffer), router,
                                   std::forward<OnAction>(on_action)...)};
}

SESSION_TEMPLATE_DECLARE
session<SESSION_TEMPLATE_ATTRIBUTES>::impl::impl(socket_type &&socket,
                                                 buffer_type &&buffer,
                                                 const router_type &router,
                                                 const on_error_type &on_error)
    : base::strand_stream{socket.get_executor()},
      m_connection{std::move(socket),
                   static_cast<base::strand_stream &>(*this)},
      m_timer{static_cast<base::strand_stream &>(*this)},
      m_buffer{std::move(buffer)},
      m_on_error{on_error},
      m_queue{*this},
      m_serializer{},
      m_parser{},
      m_dispatcher{router} {}

SESSION_TEMPLATE_DECLARE
typename session<SESSION_TEMPLATE_ATTRIBUTES>::impl::self_type &
session<SESSION_TEMPLATE_ATTRIBUTES>::impl::recv() {
  do_read();
  return *this;
}

SESSION_TEMPLATE_DECLARE
typename session<SESSION_TEMPLATE_ATTRIBUTES>::impl::self_type &
session<SESSION_TEMPLATE_ATTRIBUTES>::impl::recv(timer_duration_type duration) {
  do_timer(std::move(duration));
  do_read();
  return *this;
}

SESSION_TEMPLATE_DECLARE
template <class Message>
typename session<SESSION_TEMPLATE_ATTRIBUTES>::impl::self_type &
session<SESSION_TEMPLATE_ATTRIBUTES>::impl::send(Message &&message,
                                                 timer_duration_type duration) {
  do_timer(std::move(duration));
  m_queue(std::forward<Message>(message));
  return *this;
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::do_timer(
    timer_duration_type duration) {
  m_timer.expires_from_now(std::move(duration));
  m_timer.async_wait(std::bind(&impl::on_timer, this->shared_from_this(),
                               std::placeholders::_1));
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::on_timer(
    boost::system::error_code ec) {
  if (ec && ec != boost::asio::error::operation_aborted) {
    m_on_error(ec, "async_timer/on_timer");
    return;
  }

  if (m_timer.expiry() <= timer_type::clock_type::now()) {
    do_eof(shutdown_type::shutdown_both);
  }
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::do_read() {
  m_connection.async_read(
      m_buffer, m_parser,
      std::bind(&impl::on_read, this->shared_from_this(), std::placeholders::_1,
                std::placeholders::_2));
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::on_read(
    boost::system::error_code ec, [[maybe_unused]] size_t bytes_transferred) {
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
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::do_eof(shutdown_type type) {
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
    Message &&message) {
  m_dispatcher.do_process_request(std::forward<Message>(message), *this);
}

SESSION_TEMPLATE_DECLARE
template <bool IsMessageRequest, class MessageBody, class Fields>
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::do_write(
    boost::beast::http::message<IsMessageRequest, MessageBody, Fields>
        &message) {
  using serializer_type =
      typename details::serializer<IsMessageRequest, MessageBody>::type;

  m_serializer = std::make_any<serializer_type>(message);

  m_connection.async_write(
      std::any_cast<serializer_type &>(m_serializer),
      std::bind(&impl::on_write, this->shared_from_this(),
                std::placeholders::_1, std::placeholders::_2,
                message.need_eof()));
}

SESSION_TEMPLATE_DECLARE
void session<SESSION_TEMPLATE_ATTRIBUTES>::impl::on_write(
    boost::system::error_code ec,
    [[maybe_unused]] std::size_t bytes_transferred, bool close) {
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

SESSION_TEMPLATE_DECLARE
template <class Impl>
session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::context(Impl &impl)
    : m_impl{impl.shared_from_this()} {
  assert(m_impl != nullptr);
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::recv() {
  assert(m_impl != nullptr);
  boost::asio::dispatch(static_cast<base::strand_stream>(*m_impl),
                        std::bind(static_cast<Impl &(Impl::*)()>(&Impl::recv),
                                  m_impl->shared_from_this()));
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
template <class TimeDuration>
typename std::enable_if_t<utility::is_chrono_duration_v<TimeDuration>>
session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::recv(
    TimeDuration &&duration) {
  assert(m_impl != nullptr);
  boost::asio::dispatch(
      static_cast<base::strand_stream>(*m_impl),
      std::bind(static_cast<Impl &(Impl::*)(timer_duration_type)>(&Impl::recv),
                m_impl->shared_from_this(),
                std::chrono::duration_cast<timer_duration_type>(
                    std::forward<TimeDuration>(duration))));
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
template <class Message>
void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::send(
    Message &&message) const {
  do_send(std::forward<Message>(message), timer_duration_type::max());
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
template <class Message, class TimeDuration>
void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::send(
    Message &&message, TimeDuration &&duration) const {
  do_send(std::forward<Message>(message), std::forward<TimeDuration>(duration));
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
bool session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::is_open() const {
  assert(m_impl != nullptr);
  return m_impl->m_connection.is_open();
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
template <class Type>
Type &session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::get_user_data() & {
  return std::any_cast<Type &>(m_user_data);
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
template <class Type>
Type &&session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::get_user_data() && {
  return std::any_cast<Type &&>(std::move(m_user_data));
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
template <class Type>
void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::set_user_data(
    Type &&data) {
  m_user_data = std::make_any<Type>(std::forward<Type>(data));
}

SESSION_TEMPLATE_DECLARE
template <class Impl>
template <class Message, class TimeDuration>
void session<SESSION_TEMPLATE_ATTRIBUTES>::context<Impl>::do_send(
    Message &&message, TimeDuration &&duration) const {
  static_assert(utility::is_chrono_duration_v<TimeDuration>,
                "TimeDuration requirements are not met");

  assert(m_impl != nullptr);

  auto callback = [msg = std::forward<Message>(message),
                   dur = std::forward<TimeDuration>(duration),
                   impl = m_impl->shared_from_this()]() mutable -> Impl & {
    return impl->send(std::forward<Message>(msg),
                      std::forward<TimeDuration>(dur));
  };

  boost::asio::dispatch(static_cast<base::strand_stream &>(*m_impl),
                        std::move(callback));
}

}  // namespace beast_router
