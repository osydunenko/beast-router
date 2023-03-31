#pragma once

namespace beast_router {

static const event_loop::threads_num_type num_thrs { std::thread::hardware_concurrency() };

event_loop::event_loop()
    : m_threads_num { num_thrs }
    , m_threads {}
    , m_ioc {}
    , m_sig_int_term { m_ioc, SIGINT, SIGTERM }
{
}

ROUTER_DECL void event_loop::set_threads(threads_num_type threads)
{
    m_threads_num = threads;
}

ROUTER_DECL int event_loop::exec()
{
    int ret_code = boost::system::errc::success;
    m_sig_int_term.async_wait([this, &ret_code](const boost::system::error_code& ec, int) {
        ret_code = ec.value();
        EVENT_IOC.stop();
    });

    for (decltype(m_threads_num) cnt = 0; cnt < m_threads_num; ++cnt) {
        m_threads.emplace_back([this]() {
            EVENT_IOC.run();
        });
    }

    for (auto& thread : m_threads) {
        thread.join();
    }

    return ret_code;
}

event_loop& event_loop::get_instance()
{
    static event_loop el {};
    return el;
}

} // namespace beast_router