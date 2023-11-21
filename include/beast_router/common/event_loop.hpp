#pragma once

#include "../base/config.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/thread.hpp>
#include <thread>

ROUTER_NAMESPACE_BEGIN()

class event_loop : public std::enable_shared_from_this<event_loop> {
    template <class, class, class, template <typename> class>
    friend class listener;

    template <class, class, class, template <typename> class>
    friend class connector;

public:
    using threads_num_type = unsigned int;
    using event_loop_ptr_type = std::shared_ptr<event_loop>;

    event_loop(const event_loop&) = delete;
    event_loop& operator=(const event_loop&) = delete;
    ~event_loop();

    ROUTER_DECL int exec();
    ROUTER_DECL threads_num_type get_threads() const;
    ROUTER_DECL void stop();
    ROUTER_DECL bool is_running() const;

    template <class... Args>
    static ROUTER_DECL auto create(Args&&... args)
        -> decltype(event_loop { std::declval<Args>()... }, event_loop_ptr_type());

protected:
    event_loop(threads_num_type threads = 0u);
    ROUTER_DECL operator boost::asio::io_context&() { return m_ioc; }

private:
    threads_num_type m_threads_num;
    boost::thread_group m_threads;
    boost::asio::io_context m_ioc;
    boost::asio::signal_set m_sig_int_term;
};

ROUTER_NAMESPACE_END()

#include "impl/event_loop.ipp"