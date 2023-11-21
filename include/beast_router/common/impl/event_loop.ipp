#pragma once

#include <memory>

ROUTER_NAMESPACE_BEGIN()

event_loop::event_loop(threads_num_type threads)
    : m_threads_num { threads }
    , m_threads {}
    , m_ioc {}
    , m_sig_int_term { m_ioc, SIGINT, SIGTERM }
{
}

event_loop::~event_loop()
{
    stop();
}

ROUTER_DECL event_loop::threads_num_type
event_loop::get_threads() const
{
    return m_threads_num;
}

ROUTER_DECL void event_loop::stop()
{
    m_ioc.stop();
    m_threads.join_all();
}

ROUTER_DECL bool event_loop::is_running() const
{
    return not m_ioc.stopped();
}

ROUTER_DECL int event_loop::exec()
{
    int ret_code = boost::system::errc::success;

    m_sig_int_term.async_wait([this, &ret_code](const boost::system::error_code& ec, int) {
        ret_code = ec.value();
        m_ioc.stop();
    });

    if (m_threads_num == 0) {
        m_ioc.run();
    } else {
        m_threads.create_thread([this]() { m_ioc.run(); });
    }

    return ret_code;
}

template <class... Args>
ROUTER_DECL auto event_loop::create(Args&&... args)
    -> decltype(event_loop { std::declval<Args>()... }, event_loop_ptr_type())
{
    struct enable_make_shared : public event_loop {
        enable_make_shared(Args&&... args)
            : event_loop { std::forward<Args>(args)... }
        {
        }
    };

    return std::make_shared<enable_make_shared>(std::forward<Args>(args)...);
}

ROUTER_NAMESPACE_END()