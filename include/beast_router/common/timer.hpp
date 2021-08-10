#pragma once

#include <type_traits>
#include <boost/asio/bind_executor.hpp>

#define TIMER_TEMPLATE_ATTRIBUTES \
    CompletionExecutor, Timer

namespace beast_router {

using namespace std::chrono_literals;

/// Encapsulates the timer related functionality
/**
 * @note The class is not copyable and assignment
 */
template<
    class CompletionExecutor, 
    class Timer
>
class timer
{
public:
    /// The self type
    using self_type = timer<TIMER_TEMPLATE_ATTRIBUTES>;

    /// The timer type
    using asio_timer_type = Timer;

    /// The clock type
    using clock_type = typename asio_timer_type::clock_type;

    /// The time point type
    using time_point_type = typename clock_type::time_point;

    /// The duration type
    using duration_type = typename clock_type::duration;

    /// Constructor
    /**
     * @param executor A const reference to the complition executor
     */
    explicit timer(const CompletionExecutor &executor);

    /// Copy constructor - deleted
    timer(const self_type &) = delete;

    /// Assignment operator - deleted
    self_type &operator=(const self_type &) = delete;

    /// Move contructor - deleted
    timer(self_type &&) = delete;

    /// Move assignment operator - deleted
    self_type &operator=(self_type &&) = delete;

    /// Sets an expire time point
    /**
     * @param expire_time A duration value for the time point calculation
     * @returns The number of asynchronous operations that were cancelled
     */
    std::size_t expires_from_now(const duration_type &expire_time);

    /// Returns the expire time point
    /**
     * @return time_point_type
     */
    time_point_type expiry() const;

    /// Cancels a timer
    /**
     * @returns error_code
     */
    boost::system::error_code cancel();
 
    /// Asynchronous waiter and timer activator
    /** 
     * @param func A universal reference to the callback
     * @return void
     */
    template<class Function>
    void async_wait(Function &&func);

private:
    const CompletionExecutor &m_executor;
    asio_timer_type m_timer;
};

} // namespace beast_router

#include "impl/timer.ipp"
