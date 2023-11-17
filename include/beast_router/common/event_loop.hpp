#pragma once

#include "../base/config.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <thread>
#include <vector>

namespace beast_router {

class event_loop {
public:
    using threads_num_type = unsigned int;
    using event_loop_ptr_type = std::shared_ptr<event_loop>;

    event_loop(const event_loop&) = delete;
    event_loop(event_loop&& other) = delete;
    event_loop& operator=(const event_loop&) = delete;
    event_loop& operator=(event_loop rhs) = delete;
    ~event_loop() = default;

    ROUTER_DECL int exec();
    ROUTER_DECL threads_num_type get_threads() const;
    ROUTER_DECL void stop();
    ROUTER_DECL bool is_running() const;

    ROUTER_DECL operator boost::asio::io_context&() { return m_ioc; }

    template <class... Args>
    static ROUTER_DECL auto create(Args&&... args)
        -> decltype(event_loop { std::declval<Args>()... }, event_loop_ptr_type());

protected:
    event_loop(threads_num_type threads = 1);
    ROUTER_DECL void set_threads(threads_num_type threads);

private:
    threads_num_type m_threads_num;
    std::vector<std::thread> m_threads;
    boost::asio::io_context m_ioc;
    boost::asio::signal_set m_sig_int_term;
};

} // namespace beast_router

#include "impl/event_loop.ipp"