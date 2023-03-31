#pragma once

#include "../base/config.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <thread>
#include <vector>

#define EVENT_LOOP \
    []() -> beast_router::event_loop& { return beast_router::event_loop::get_instance(); }()
#define EVENT_IOC \
    static_cast<boost::asio::io_context&>(beast_router::event_loop::get_instance())

namespace beast_router {

class event_loop {
public:
    using threads_num_type = unsigned int;

    event_loop(const event_loop&) = delete;
    event_loop(event_loop&&) = delete;
    event_loop& operator=(const event_loop&) = delete;
    event_loop& operator=(event_loop) = delete;
    ~event_loop() = default;

    ROUTER_DECL operator boost::asio::io_context&() { return m_ioc; }

    ROUTER_DECL int exec();

    static event_loop& get_instance();

protected:
    event_loop();

    ROUTER_DECL void set_threads(threads_num_type threads);

private:
    threads_num_type m_threads_num;
    std::vector<std::thread> m_threads;
    boost::asio::io_context m_ioc;
    boost::asio::signal_set m_sig_int_term;
};

} // namespace beast_router

#include "impl/event_loop.ipp"