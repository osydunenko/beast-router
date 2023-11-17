#pragma once

#include "beast_router/common/event_loop.hpp"
#include "beast_router/listener.hpp"
#include "beast_router/server.hpp"
#include "beast_router/session.hpp"

namespace beast_router {

/// The Http Server type
using http_server = server<server_session, http_listener, event_loop>;

} // namespace beast_router
