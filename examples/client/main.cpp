#include "beast_router.hpp"
#include <iostream>

using namespace std::chrono_literals;

int main(int, char**)
{
    /// Create event loop
    beast_router::event_loop::event_loop_ptr_type event_loop = beast_router::event_loop::create();

    /// Declare on connect callback
    beast_router::http_connector_type::on_connect_type on_connect = [event_loop](beast_router::http_connector_type::socket_type socket) {
        beast_router::http::request<beast_router::http::empty_body> req { beast_router::http::verb::get, "/", 11 };
        req.set(beast_router::http::field::host, "wttr.in");
        req.set(beast_router::http::field::user_agent, "curl");

        /// routing table
        beast_router::http_client_type::router_type router;
        router.handle_response([event_loop](const beast_router::http_client_type::message_type& rp,
                                   beast_router::http_client_type::context_type&) {
            /// Print the received data and stop the loop
            std::cout << rp << std::endl;
            event_loop->stop();
        });

        /// On error handler
        beast_router::http_connector_type::on_error_type on_error = [event_loop](boost::system::error_code,
                                                                   std::string_view msg) {
            std::cerr << "Error: " << msg << std::endl;
            event_loop->stop();
        };

        /// Send the request
        beast_router::http_client_type::send(std::move(socket), std::move(req), router,
            std::move(on_error))
            .recv();
    };

    /// On connect error handler
    beast_router::http_connector_type::on_error_type on_error = [event_loop](boost::system::error_code ec,
                                                               std::string_view msg) {
        std::cerr << "On connect error: " << msg << ":" << ec << std::endl;
        event_loop->stop();
    };

    /// Connect to the host and send the request
    beast_router::http_connector_type::connect(*event_loop, "wttr.in", "80",
        std::move(on_connect), std::move(on_error));

    /// Start serving
    return event_loop->exec();
}
