#pragma once

#include <thread>

#include <boost/asio/signal_set.hpp>

#include "beast_router/listener.hpp"
#include "beast_router/session.hpp"
#include "beast_router/router.hpp"
#include "beast_router/connector.hpp"

#include "beast_router/common/http_utility.hpp"

namespace beast_router {

template<class Session>
struct handlers: public Session
{
    using session_type = Session;

    using router_type = router<session_type>;

    using context_type = typename session_type::context_type;

    using message_type = typename session_type::message_type;
};

/// The default listener type
using http_listener = listener<>;

/// The connector type
using http_connector = connector<>;

/// The server connections handler
using http_server = handlers<session<true>>;

/// The client connections handler
using http_client = handlers<session<false>>;

} // namespace beast_router
