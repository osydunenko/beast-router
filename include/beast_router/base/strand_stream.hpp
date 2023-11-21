#pragma once

#include <boost/asio/strand.hpp>
#include <boost/asio/system_timer.hpp>

#include "config.hpp"

ROUTER_BASE_NAMESPACE_BEGIN()

/// Base class exposes a strand stream for the multi threading applications
struct strand_stream
    : boost::asio::strand<boost::asio::system_timer::executor_type> {
    using asio_type = boost::asio::strand<boost::asio::system_timer::executor_type>;

    strand_stream(const boost::asio::system_timer::executor_type& executor)
        : asio_type { executor }
    {
    }
};

ROUTER_BASE_NAMESPACE_END()
