#pragma once

#include "listener.hpp"
#include "router.hpp"
#include "session.hpp"
#include <thread>
#include <vector>

namespace beast_router {

class server {
public:
    using threads_num_type = unsigned int;
    using address_type = boost::asio::ip::address;
    using port_type = unsigned short;
    using duration_type = std::chrono::milliseconds;
    using router_type = server_session::router_type;
    using message_type = server_session::message_type;

    server();
    server(server const&) = delete;
    server(server&&) = delete;
    server& operator=(server const&) = delete;
    server& operator=(server) = delete;
    virtual ~server() = default;

    [[nodiscard]] inline int exec();

    inline void set_threads(threads_num_type threads);
    [[nodiscard]] inline threads_num_type threads() const;

    inline void set_address(const std::string& address);
    [[nodiscard]] inline std::string address() const;

    inline void set_port(port_type port);
    [[nodiscard]] inline port_type port() const;

    inline void set_recv_timeout(duration_type timeout);
    [[nodiscard]] inline duration_type recv_timeout() const;

    template <class... OnRequest>
    server& on_get(const std::string& path, OnRequest&&... actions);

    template <class... OnRequest>
    server& on_put(const std::string& path, OnRequest&&... actions);

    template <class... OnRequest>
    server& on_post(const std::string& path, OnRequest&&... actions);

    template <class... OnRequest>
    server& on_delete(const std::string& path, OnRequest&&... actions);

private:
    std::vector<std::thread> m_threads;
    boost::asio::io_context m_ioc;
    boost::asio::signal_set m_sig_int_term;
    router_type m_router;
    threads_num_type m_threads_num;
    address_type m_address;
    port_type m_port;
    duration_type m_recv_timeout;
};

} // namespace beast_router

#include "impl/server.ipp"