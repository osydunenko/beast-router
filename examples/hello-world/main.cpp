#include "beast_router.hpp"
#include <atomic>
#include <sstream>

/// requests counter
static std::atomic<uint64_t> counter { 0 };

int main(int, char**)
{
    return beast_router::http_server()
        .on<beast_router::http_server::get_t>(R"(^.*$)", [](const auto& rq, auto& ctx) {
            std::stringstream i_str;
            i_str << "Hello World: the request was triggered [" << ++counter
                  << "] times";

            auto rp = beast_router::make_string_response(beast_router::http::status::ok, rq.version(), i_str.str());
            rp.keep_alive(rq.keep_alive());

            ctx.send(std::move(rp));
        })
        .exec();
}
