#pragma once

#include "../base/config.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/thread.hpp>
#include <thread>

ROUTER_NAMESPACE_BEGIN()

/// Encapsulates the event loop and handles further asio requests
/**
 * The class serves as a wrapper around `boost::asio::io_context`
 * By the giveb number of threads, create a thread pool and executes 
 * handlers by leveraging their executors. This class is acceptable
 * by the @ref session and @connector.
 */
class event_loop : public std::enable_shared_from_this<event_loop> {
    template <class, class, class, template <typename> class>
    friend class listener;

    template <class, class, class, template <typename> class>
    friend class connector;

public:
    /// The `threads number` type
    using threads_num_type = unsigned int;

    /// This `pointer` type
    using event_loop_ptr_type = std::shared_ptr<event_loop>;

    /// Constructor (disallowed)
    event_loop(const event_loop&) = delete;

    /// Constructor (disallowed)
    event_loop& operator=(const event_loop&) = delete;

    /// Constructor (disallowed)
    event_loop(event_loop&&) = delete;

    /// Assignment (disallowed)
    event_loop& operator=(event_loop&&) = delete;

    /// Destructor
    ~event_loop();

    /// Starts the event loop
    /**
     * @returns void
    */
    ROUTER_DECL int exec();

    /// Returns the number of thread
    /**
     * @returns @ref threads_num_type
    */
    ROUTER_DECL threads_num_type get_threads() const;

    /// Stops the event loop
    /**
     * @returns void
    */
    ROUTER_DECL void stop();

    /// Retruns the running state of event loop
    /**
     * @returns bool
    */
    ROUTER_DECL bool is_running() const;

    /// The method creates an event loop by the given parameters
    /**
     * The mthod accepts a pack and forwards to the available 
     * hidden constructor
     * @param args...
     * @returns @ref event_loop_ptr_type
     */
    template <class... Args>
    static ROUTER_DECL auto create(Args&&... args)
        -> decltype(event_loop { std::declval<Args>()... }, event_loop_ptr_type());

    /// The static cast operator reterns the current context
    /**
     * @returns boost::asio::io_context&
    */
    ROUTER_DECL operator boost::asio::io_context&() { return m_ioc; }

protected:
    event_loop(threads_num_type threads = 0u);

private:
    threads_num_type m_threads_num;
    boost::thread_group m_threads;
    boost::asio::io_context m_ioc;
    boost::asio::signal_set m_sig_int_term;
};

ROUTER_NAMESPACE_END()

#include "impl/event_loop.ipp"
