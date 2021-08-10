#include <atomic>
#include <vector>
#include <sstream>
#include "beast_router.hpp"

using namespace std;
using namespace boost;
using namespace beast_router;

/// Global requests counter
static atomic<uint64_t> counter{0};

/// Global ip address
static const auto address = boost::asio::ip::address_v4::any();

/// Global port to start listening on
static const auto port = static_cast<unsigned short>(8081);

/// Global io_context
static asio::io_context ioc;

/// Global signals termination
static asio::signal_set sig_int_term{ioc, SIGINT, SIGTERM};

/// Global routing table
static beast_router::http_router router;

int main(int argc, char **argv)
{
    /// Define the callback
    auto clb = [](const beast_http_request &rq, http_context &ctx, const std::smatch &) {
        beast_string_response rp{beast::http::status::ok, rq.version()};
        rp.set(beast::http::field::content_type, "text/html");

        stringstream i_str;
        i_str << "Hello World has been called [" << ++counter << "] times";

        rp.body() = i_str.str();
        rp.prepare_payload();
        rp.keep_alive(rq.keep_alive());

        ctx.send(rp);
    };

    /// Add the above callback and link to all the possible requests patterns
    ::router.get(R"(^.*$)", std::move(clb));

    /// Define callbacks for the listener
    http_listener::on_error_type on_error = [](system::error_code ec, boost::string_view v) {
        if (ec == system::errc::address_in_use ||
            ec == system::errc::permission_denied)
            ioc.stop();
    };
    http_listener::on_accept_type on_accept = [&on_error](http_listener::socket_type socket) {
        http_session::recv(std::move(socket), ::router, 5s, on_error);
    };

    /// Start listening
    http_listener::launch(ioc, {address, port}, std::move(on_accept), std::move(on_error));

    /// Declare interruptions
    sig_int_term.async_wait([](const system::error_code &, int){
        ioc.stop();
    });

    /// Get number of available processors
    auto processor_count = thread::hardware_concurrency();

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
