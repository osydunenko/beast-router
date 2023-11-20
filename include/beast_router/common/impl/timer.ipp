#pragma once

#define TIMER_TEMPLATE_DECLARE template <class CompletionExecutor, class Timer>

ROUTER_NAMESPACE_BEGIN()

TIMER_TEMPLATE_DECLARE
timer<TIMER_TEMPLATE_ATTRIBUTES>::timer(const CompletionExecutor& executor)
    : m_executor { executor }
    , m_timer { m_executor, (asio_timer_type::time_point::max)() }
{
}

TIMER_TEMPLATE_DECLARE
std::size_t timer<TIMER_TEMPLATE_ATTRIBUTES>::expires_from_now(
    const duration_type& expire_time)
{
    return m_timer.expires_after(expire_time);
}

TIMER_TEMPLATE_DECLARE
typename timer<TIMER_TEMPLATE_ATTRIBUTES>::time_point_type
timer<TIMER_TEMPLATE_ATTRIBUTES>::expiry() const
{
    return m_timer.expiry();
}

TIMER_TEMPLATE_DECLARE
boost::system::error_code timer<TIMER_TEMPLATE_ATTRIBUTES>::cancel()
{
    boost::system::error_code ec {};
    try {
        m_timer.cancel();
    } catch (boost::system::system_error& ex) {
        ec = ex.code();
    }
    return ec;
}

TIMER_TEMPLATE_DECLARE
template <class Function>
void timer<TIMER_TEMPLATE_ATTRIBUTES>::async_wait(Function&& func)
{
    static_assert(std::is_invocable_v<Function, boost::system::error_code>,
        "timer::async_wait requirements are not met");
    m_timer.async_wait(
        boost::asio::bind_executor(m_executor, std::forward<Function>(func)));
}

ROUTER_NAMESPACE_END()
