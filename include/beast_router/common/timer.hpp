#pragma once

#include <type_traits>
#include <boost/asio/bind_executor.hpp>

#define TIMER_TEMPLATE_ATTRIBUTES \
    CompletionExecutor, Timer

namespace beast_router {

using namespace std::chrono_literals;

template<
    class CompletionExecutor, 
    class Timer
>
class timer
{
public:
    using self_type = timer<TIMER_TEMPLATE_ATTRIBUTES>;

    using asio_timer_type = Timer;

    using clock_type = typename asio_timer_type::clock_type;

    using time_point_type = typename clock_type::time_point;

    using duration_type = typename clock_type::duration;

    explicit timer(const CompletionExecutor &executor);
    timer(const self_type &) = delete;
    self_type &operator=(const self_type &) = delete;
    timer(self_type &&) = delete;
    self_type &operator=(self_type &&) = delete;

    std::size_t expires_from_now(const duration_type &expire_time);
    time_point_type expiry() const;

    boost::system::error_code cancel();

    template<class Function>
    void async_wait(Function &&func);

private:
    const CompletionExecutor &m_executor;
    asio_timer_type m_timer;
};

} // namespace beast_router

#include "impl/timer.hpp"
