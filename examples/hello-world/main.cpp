#include <atomic>
#include <iostream>
#include <sstream>
#include <vector>

#include "beast_router.hpp"

using namespace std;
using namespace boost;
using namespace beast_router;

/// requests counter
static atomic<uint64_t> counter{0};

/// ip address
static const auto address = asio::ip::address_v4::any();

/// port to start listening on
static const auto port = static_cast<unsigned short>(8081);

/// io_context
static asio::io_context ioc;

/// signals termination
static asio::signal_set sig_int_term{ioc, SIGINT, SIGTERM};

/// routing table
static server_session::router_type router;

int main(int, char **) {
  /// Define the callback
  auto clb = [](const server_session::message_type &rq,
                server_session::context_type &ctx) {
    stringstream i_str;
    i_str << "Hello World: the request was triggered [" << ++counter
          << "] times";

    auto rp = make_string_response(http::status::ok, rq.version(), i_str.str());
    rp.keep_alive(rq.keep_alive());

    ctx.send(std::move(rp));
  };

  /// Add the above callback and link to all the possible requests patterns
  ::router.get(R"(^.*$)", std::move(clb));

  /// Define callbacks for the listener
  http_listener::on_error_type on_error = [](system::error_code ec,
                                             std::string_view) {
    if (ec == system::errc::address_in_use ||
        ec == system::errc::permission_denied)
      ioc.stop();
  };
  http_listener::on_accept_type on_accept =
      [&on_error](http_listener::socket_type socket) {
        server_session::recv(std::move(socket), ::router, 5s, on_error);
      };

  /// Start listening
  http_listener::launch(ioc, {address, port}, std::move(on_accept),
                        std::move(on_error));

  /// Declare interruptions
  sig_int_term.async_wait([](const system::error_code &, int) { ioc.stop(); });

  /// Get number of available processors
  auto processor_count = thread::hardware_concurrency();

  /// Create threads where size is eq. to the number of CPUs
  vector<thread> threads;
  for (decltype(processor_count) i = 0; i < processor_count; ++i) {
    threads.emplace_back(bind(
        static_cast<size_t (asio::io_context::*)()>(&asio::io_context::run),
        std::ref(ioc)));
  }

  /// Join threads and quit
  for (auto &thread : threads) {
    thread.join();
  }

  return 0;
}
