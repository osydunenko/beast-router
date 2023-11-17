#include "beast_router.hpp"
#include "config_ex.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>

int main(int, char**)
{
    return beast_router::http_server()
        .on<beast_router::http_server::get_t>(R"(^.*\.(js|html|css)$)", [](const auto& rq, auto& ctx) {
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
        })
        .on<beast_router::http_server::get_t>(R"(^/$)", [](const auto& rq, auto& ctx) {
            ctx.send(beast_router::make_moved_response(rq.version(), "/index.html"));
        })
        .on<beast_router::http_server::get_t>(R"(^/update$)", [](const auto& rq, auto& ctx) {
            auto now = std::chrono::system_clock::now();
            auto itt = std::chrono::system_clock::to_time_t(now);

            std::stringstream i_str;
            i_str << std::put_time(std::gmtime(&itt), "%FT%TZ");
            ctx.send(beast_router::make_string_response(beast_router::http::status::ok, rq.version(), i_str.str()));
        })
        .exec();
}
