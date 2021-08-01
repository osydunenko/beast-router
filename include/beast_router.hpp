#pragma once

#include <thread>
#include <boost/asio/signal_set.hpp>
#include <boost/beast/http/message.hpp>

#include "beast_router/listener.hpp"
#include "beast_router/session.hpp"
#include "beast_router/router.hpp"

namespace beast_router {

using http_listener = listener<>;
using http_session = session<>;
using http_router = router<http_session>;

using http_context = typename http_session::context_type;
using beast_http_request = typename http_session::request_type;

template<class Body>
using beast_http_response = boost::beast::http::response<Body>;
using beast_string_response = beast_http_response<boost::beast::http::string_body>;

static boost::asio::io_context g_ioc;

static boost::asio::signal_set sig_int_term{g_ioc, SIGINT, SIGTERM};

static http_router g_router;

} // namespace beast_router
