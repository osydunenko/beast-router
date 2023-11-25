#include "beast_router.hpp"
#include "config_ex.hpp"
#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>

using namespace std::chrono_literals;

/// ip address
static const auto address = boost::asio::ip::address_v4::any();

/// port to start listening on
static const auto port = static_cast<unsigned short>(8080);

int main(int, char**)
{
    /// routing table
    beast_router::http_server_type::router_type router {};

    /// Add the callback and link to the concerned requests patterns
    router.get(R"(^.*\.(js|html|css)$)", [](const auto& rq, auto& ctx) {
        // Request path must be absolute and not contain ".."
        if (rq.target().empty() || rq.target()[0] != '/' || rq.target().find("..") != boost::beast::string_view::npos) {
            // Send bad request
            return ctx.send(beast_router::make_string_response(beast_router::http::status::bad_request,
                rq.version(),
                "Illegal request-target"));
        }

        // Append an HTTP rel-path to a local fs path
        std::string path { DOC_BASE_PATH };
        path.append(rq.target().data(), rq.target().size());

        // Attempt to open a file and respond to the request
        auto rp = beast_router::make_file_response(rq.version(), path);
        if (rp.result() != beast_router::http::status::ok) {
            return ctx.send(beast_router::make_string_response(beast_router::http::status::not_found,
                rq.version(), "Not Found"));
        }

        return ctx.send(std::move(rp));
    });
    router.get(R"(^/$)", [](const auto& rq, auto& ctx) {
        ctx.send(beast_router::make_moved_response(rq.version(), "/index.html"));
    });
    router.get(R"(^/update$)", [](const auto& rq, auto& ctx) {
        auto now = std::chrono::system_clock::now();
        auto itt = std::chrono::system_clock::to_time_t(now);

        std::stringstream i_str;
        i_str << std::put_time(std::gmtime(&itt), "%FT%TZ");
        ctx.send(beast_router::make_string_response(beast_router::http::status::ok, rq.version(), i_str.str()));
    });

    /// Create event loop to serve the io context
    auto event_loop = beast_router::event_loop::create();

    /// Define callbacks for the listener
    beast_router::http_listener_type::on_error_type on_error = [event_loop](boost::system::error_code ec,
                                                                   std::string_view) {
        throw std::runtime_error(ec.message());
    };

    beast_router::http_listener_type::on_accept_type on_accept = [&on_error, &router](beast_router::http_listener_type::socket_type socket) {
        beast_router::http_server_type::recv(std::piecewise_construct, std::move(socket), router, 1s, on_error);
    };

    /// Start listening
    beast_router::http_listener_type::launch(*event_loop, { address, port }, std::move(on_accept), std::move(on_error));

    /// Start serving
    return event_loop->exec();
}
