#include <thread>
#include <iostream>

#include "listener.hpp"
#include "session.hpp"
#include "router.hpp"
#include "base/lockable.hpp"

#include <boost/asio/signal_set.hpp>
#include <boost/beast/http/message.hpp>

static boost::asio::io_context ioc;
static boost::asio::signal_set sig_int_term{ioc, SIGINT, SIGTERM};
static server::base::lockable::mutex_type mutex;  

int main()
{
    using http_listener = server::default_listener;
    using http_session = server::default_session;
    using http_router = server::router<http_session>;

    using http_context = typename http_session::context_type;
    using beast_http_request = typename http_session::request_type;
    using beast_http_response = boost::beast::http::response<boost::beast::http::string_body>;

    http_router router;

    router.get(R"(^/.*$)", 
        [](const beast_http_request &req, http_context &ctx, const std::smatch &match) {
            LOCKABLE_ENTER_TO_WRITE(mutex);
            std::clog << "RQ: method by thread id: " << std::this_thread::get_id() << std::endl; 
            ctx.set_user_data<std::string>("Hello World !!!");
            return true; // Propaget events further
        }, 
        [](const beast_http_request &req, http_context &ctx, const std::smatch &match) {
            beast_http_response rsp{boost::beast::http::status::ok, req.version()};
            rsp.set(boost::beast::http::field::content_type, "text/html");
            rsp.body() = ctx.get_user_data<std::string&>().second;
            rsp.prepare_payload();
            rsp.keep_alive(req.keep_alive());
            ctx.send(rsp);
        }
    );

    http_listener::on_error_type on_error = [](boost::system::error_code ec, boost::string_view v) {
        if (ec == boost::system::errc::address_in_use ||
            ec == boost::system::errc::permission_denied) {
            ioc.stop();
        }
    };

    http_listener::on_accept_type on_accept = [&](http_listener::socket_type socket) {
        http_session::recv(std::move(socket), router, on_error);
    };

    const auto address = boost::asio::ip::address_v4::any();
    const auto port = static_cast<unsigned short>(8080);

    // Start accepting
    http_listener::launch(ioc, {address, port}, on_accept, on_error);
    sig_int_term.async_wait([](boost::system::error_code const&, int sig) {
        ioc.stop();
    });

    uint32_t pool_size = std::thread::hardware_concurrency() * 2;

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> threads;
    threads.reserve(pool_size > 0 ? pool_size : 4);
    for(uint32_t i = 0; i < pool_size; i++)
        threads.emplace_back(std::bind(static_cast<std::size_t (boost::asio::io_context::*)()>
                                       (&boost::asio::io_context::run), std::ref(ioc)));

    for(auto &thr : threads)
        thr.join();

    return 0;
}
