#pragma once

#include <boost/asio/strand.hpp>
#include <boost/asio/system_timer.hpp>

namespace beast_router {
namespace base {

struct strand_stream: boost::asio::strand<boost::asio::system_timer::executor_type>
{
    using asio_type = boost::asio::strand<boost::asio::system_timer::executor_type>;

    strand_stream(const boost::asio::system_timer::executor_type &executor)
        : asio_type(executor)
    {
    }
};

} // namespace base
} // namespace beast_router
