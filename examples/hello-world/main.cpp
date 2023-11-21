#include "beast_router.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <sstream>

using namespace std::chrono_literals;

/// requests counter
static std::atomic<uint64_t> counter { 0 };

/// ip address
static const auto address = boost::asio::ip::address_v4::any();

/// port to start listening on
static const auto port = static_cast<unsigned short>(8080);

int main(int, char**)
{
    /// routing table
    beast_router::http_server_type::router_type router {};

    /// Add the callback and link to index ("/") requests patterns
    router.get(R"(^/$)", [](const beast_router::http_server_type::message_type& rq, beast_router::http_server_type::context_type& ctx) {
        const auto now = std::chrono::system_clock::now();
        const std::time_t t_c = std::chrono::system_clock::to_time_t(now);
        std::cout << "Received RQ. at " << std::ctime(&t_c);

        std::stringstream i_str;
        i_str << "Hello World: the request was triggered [" << ++counter
              << "] times";
        auto rp = beast_router::make_string_response(beast_router::http::status::ok, rq.version(), i_str.str());
        rp.keep_alive(rq.keep_alive());

        ctx.send(std::move(rp));
    });

    /// Create event loop to serve the io context
    auto event_loop = beast_router::event_loop::create();

    /// Define callbacks for the listener
    beast_router::http_listener_type::on_error_type on_error = [event_loop](boost::system::error_code ec,
                                                                   std::string_view) {
        if (ec == boost::system::errc::address_in_use || ec == boost::system::errc::permission_denied) {
            std::cerr << "Address in use or permission denied" << std::endl;
            event_loop->stop();
        }
    };

    beast_router::http_listener_type::on_accept_type on_accept = [&on_error, &router](beast_router::http_listener_type::socket_type socket) {
        beast_router::http_server_type::recv(std::piecewise_construct, std::move(socket), router, 1s, on_error);
    };

    /// Start listening
    beast_router::http_listener_type::launch(*event_loop, { address, port }, std::move(on_accept), std::move(on_error));

    /// Start serving
    return event_loop->exec();
}
