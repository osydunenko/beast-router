#include <string_view>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <map>

#include "beast_router.hpp"
#include "config.hpp"

using namespace boost;
using namespace beast_router;

/// ip address
static const auto address = asio::ip::address_v4::any();

/// port to start listening on
static const auto port = static_cast<unsigned short>(8081);

/// io_context
static asio::io_context ioc;

/// signals termination
static asio::signal_set sig_int_term{ioc, SIGINT, SIGTERM};

/// routing table
static http_server::router_type router;

int main(int, char **)
{
    /// Add handler for the static content
    ::router.get(R"(^.*\.(js|html|css)$)", 
        [](const http_server::message_type &rq, http_server::context_type &ctx) {
            // Request path must be absolute and not contain ".."
            if (rq.target().empty() ||
                rq.target()[0] != '/' ||
                rq.target().find("..") != beast::string_view::npos) {
                
                // Send bad request
                return ctx.send(make_string_response(http::status::bad_request, 
                    rq.version(), "Illegal request-target"));
            }
            
            // Append an HTTP rel-path to a local fs path
            std::string path{DOC_BASE_PATH};
            path.append(rq.target().data(), rq.target().size());

            // Attempt to open a file and respond to the request
            auto rp = make_file_response(rq.version(), path);
            if (rp.result() != http::status::ok) {
                return ctx.send(make_string_response(http::status::not_found,
                    rq.version(), "Not Found"));
            }
        
            return ctx.send(std::move(rp));
        });

    /// Redirect to index.html
    ::router.get(R"(^/$)", [](const http_server::message_type &rq, http_server::context_type &ctx) {
        ctx.send(make_moved_response(rq.version(), "/index.html"));
    });

    /// Handle the "update" request
    ::router.get(R"(^/update$)", [](const http_server::message_type &rq, http_server::context_type &ctx) {
        auto now = std::chrono::system_clock::now();
        auto itt = std::chrono::system_clock::to_time_t(now);

        std::stringstream i_str;
        i_str << std::put_time(std::gmtime(&itt), "%FT%TZ");
        ctx.send(make_string_response(http::status::ok,
                    rq.version(), i_str.str()));
    });

    /// Define callbacks for the listener
    http_listener::on_error_type on_error = [](system::error_code ec, std::string_view) {
        if (ec == system::errc::address_in_use ||
            ec == system::errc::permission_denied) {
                std::cout << "An error occured: cannot bind to..." << std::endl;
                ioc.stop();
        }
    };
    http_listener::on_accept_type on_accept = [&on_error](http_listener::socket_type socket) {
        http_server::recv(std::move(socket), ::router, 5s, on_error);
    };

    /// Start listening
    http_listener::launch(ioc, {address, port}, std::move(on_accept), std::move(on_error));

    /// Declare interruptions
    sig_int_term.async_wait([](const system::error_code &, int){
        ioc.stop();
    });

    /// Get number of available processors
    auto processor_count = std::thread::hardware_concurrency();

    /// Create threads where size is eq. to the number of CPUs
    std::vector<std::thread> threads;
    for (decltype(processor_count) i = 0; i < processor_count; ++i) {
        threads.emplace_back(
            bind(static_cast<size_t(asio::io_context::*)()>(&asio::io_context::run), std::ref(ioc))
        );
    }

    /// Join threads and quit
    for (auto &thread : threads) {
        thread.join();
    }

    return 0;
}
