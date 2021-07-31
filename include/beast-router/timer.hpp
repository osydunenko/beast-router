#pragma once

#include <type_traits>
#include <boost/asio/bind_executor.hpp>

#define TIMER_TEMPLATE_DECLARE \
    template<                  \
    class CompletionExecutor,  \
    class Timer                \
    >

#define TIMER_TEMPLATE_ATTRIBUTES \
    CompletionExecutor, Timer

namespace beast_router {

template<
    class CompletionExecutor, 
    class Timer
>
class timer
{
public:
    using self_type = timer<TIMER_TEMPLATE_ATTRIBUTES>;

    using asio_timer_type = Timer;

    using time_point_type = typename asio_timer_type::clock_type::time_point;

    using duration_type = typename asio_timer_type::clock_type::duration;

    explicit timer(const CompletionExecutor &executor);
    timer(const self_type &) = delete;
    self_type &operator=(const self_type &) = delete;
    timer(self_type &&) = delete;
    self_type &operator=(self_type &&) = delete;

    std::size_t expires_at(const time_point_type &expire_time);
    std::size_t expires_from_now(const duration_type &expire_time);

    boost::system::error_code cancel();

    template<class Function>
    void async_wait(Function &&func);

private:
    const CompletionExecutor &m_executor;
    asio_timer_type m_timer;
};

TIMER_TEMPLATE_DECLARE
timer<TIMER_TEMPLATE_ATTRIBUTES>::timer(const CompletionExecutor &executor)
    : m_executor{executor}
    , m_timer{m_executor, (asio_timer_type::time_point::max)()}
{}

TIMER_TEMPLATE_DECLARE
std::size_t timer<TIMER_TEMPLATE_ATTRIBUTES>::expires_at(const time_point_type &expire_time)
{
    return m_timer.expires_at(expire_time);
}

TIMER_TEMPLATE_DECLARE
std::size_t timer<TIMER_TEMPLATE_ATTRIBUTES>::expires_from_now(const duration_type &expire_time)
{
    return m_timer.expires_from_now(expire_time);
}

TIMER_TEMPLATE_DECLARE
boost::system::error_code timer<TIMER_TEMPLATE_ATTRIBUTES>::cancel()
{
    boost::system::error_code ec{};
    try {
        m_timer.cancel();
    } catch (boost::system::system_error &ex) {
        ec = ex.code();
    }
    return ec;
}

TIMER_TEMPLATE_DECLARE
template<class Function>
void timer<TIMER_TEMPLATE_ATTRIBUTES>::async_wait(Function &&func)
{
    static_assert(std::is_invocable_v<Function, boost::system::error_code>);
    m_timer->async_wait(
        boost::asio::bind_executor(
            m_executor, std::forward<Function>(func)
        )
    );
}

} // namespace beast_router
