#pragma once

#include <thread>

#include <boost/asio/signal_set.hpp>

#include "beast_router/listener.hpp"
#include "beast_router/session.hpp"
#include "beast_router/router.hpp"

#include "beast_router/common/http_utility.hpp"

namespace beast_router {

/// The default listener type
using http_listener = listener<>;

/// The default session type
using http_session = session<>;

/// The default router type
using http_router = router<http_session>;

/// HTTP context
using http_context = typename http_session::context_type;

/// Beast HTTP Request type
using beast_http_request = typename http_session::request_type;

} // namespace beast_router
