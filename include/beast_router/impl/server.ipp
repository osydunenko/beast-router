#pragma once

namespace beast_router {

static const server::address_type default_address { boost::asio::ip::address_v4::any() };
static const server::threads_num_type num_thrs { std::thread::hardware_concurrency() };
static const server::port_type default_port { 8080 };
static const server::duration_type default_recv_timeout { 5s };

server::server()
    : m_threads {}
    , m_ioc {}
    , m_sig_int_term { m_ioc, SIGINT, SIGTERM }
    , m_router {}
    , m_threads_num { num_thrs }
    , m_address { default_address }
    , m_port { default_port }
    , m_recv_timeout { default_recv_timeout }
{
}

inline int server::exec()
{
    int ret_code = 0;
    m_sig_int_term.async_wait([this, &ret_code](const boost::system::error_code& ec, int) {
        ret_code = ec.value();
        m_ioc.stop();
    });

    auto on_accept = [this](http_listener::socket_type socket) {
        server_session::recv(std::move(socket), m_router, recv_timeout(),
            [](boost::system::error_code, std::string_view) {});
    };

    auto on_error = [this, &ret_code](boost::system::error_code ec, std::string_view) {
        if (ec == boost::system::errc::address_in_use or ec == boost::system::errc::permission_denied) {
            ret_code = ec.value();
            m_ioc.stop();
        }
    };

    http_listener::launch(m_ioc, { m_address, m_port }, std::move(on_accept), std::move(on_error));

    auto thrs = threads();
    for (decltype(thrs) cnt = 0; cnt < thrs; ++cnt) {
        m_threads.emplace_back([this]() {
            m_ioc.run();
        });
    }

    for (auto& thread : m_threads) {
        thread.join();
    }

    return ret_code;
}

inline void server::set_threads(threads_num_type threads)
{
    m_threads_num = threads;
}

inline server::threads_num_type server::threads() const
{
    return m_threads_num;
}

inline void server::set_address(const std::string& address)
{
    m_address = address_type::from_string(address);
}

inline std::string server::address() const
{
    return m_address.to_string();
}

inline void server::set_port(port_type port)
{
    m_port = port;
}

inline server::port_type server::port() const
{
    return m_port;
}

inline void server::set_recv_timeout(duration_type timeout)
{
    m_recv_timeout = timeout;
}

inline server::duration_type server::recv_timeout() const
{
    return m_recv_timeout;
}

template <class... OnRequest>
server& server::on_get(const std::string& path, OnRequest&&... actions)
{
    m_router.get(path, std::forward<OnRequest>(actions)...);
    return *this;
}

template <class... OnRequest>
server& server::on_put(const std::string& path, OnRequest&&... actions)
{
    m_router.put(path, std::forward<OnRequest>(actions)...);
    return *this;
}

template <class... OnRequest>
server& server::on_post(const std::string& path, OnRequest&&... actions)
{
    m_router.post(path, std::forward<OnRequest>(actions)...);
    return *this;
}

template <class... OnRequest>
server& server::on_delete(const std::string& path, OnRequest&&... actions)
{
    m_router.delete_(path, std::forward<OnRequest>(actions)...);
    return *this;
}

} // namespace beast_router