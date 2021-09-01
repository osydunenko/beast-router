#pragma once

#include <thread>

#include <boost/asio/signal_set.hpp>

#include "beast_router/listener.hpp"
#include "beast_router/session.hpp"
#include "beast_router/router.hpp"
#include "beast_router/connector.hpp"

#include "beast_router/common/http_utility.hpp"

namespace beast_router {

/// The default listener type
using http_listener = listener<>;

/// The connector type
using http_connector = connector<>;

/// The default http server session type
using http_server_session = session<>;

/// The default server router type
using http_server_router = router<http_server_session>;

/// HTTP context
using http_server_context = typename http_server_session::context_type;

/// HTTP server Request type
using http_server_request = typename http_server_session::request_type;

} // namespace beast_router
