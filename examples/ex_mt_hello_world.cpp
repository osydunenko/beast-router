#include <string_view>
#include <sstream>

#include "beast_router.hpp"

using namespace beast_router;

int main()
{
    auto clb = [](const beast_http_request &rq, http_context &ctx, const std::smatch &match) {
        static const std::string_view greeting = "Hello World!";
        ctx.set_user_data(greeting);
        return true; // Propagate the request further
    };

    auto snd = [](const beast_http_request &rq, http_context &ctx, const std::smatch &match) {
        beast_string_response rp{boost::beast::http::status::ok, rq.version()};
        rp.set(boost::beast::http::field::content_type, "text/html");

        std::stringstream i_str;
        i_str << 
            ctx.get_user_data<std::string_view &>() << " " <<
            "Generated by the thread: " << std::this_thread::get_id();

        rp.body() = i_str.str();

        rp.prepare_payload();
        rp.keep_alive(rq.keep_alive());
        ctx.send(rp);
    };

    g_router.get(R"(^/.*$)", std::move(clb), std::move(snd));

    http_listener::on_error_type on_error = [](boost::system::error_code ec, boost::string_view v) {
        if (ec == boost::system::errc::address_in_use ||
            ec == boost::system::errc::permission_denied)
            g_ioc.stop();
    };

    http_listener::on_accept_type on_accept = [&on_error](http_listener::socket_type socket) {
        http_session::recv(std::move(socket), g_router, 5s, on_error);
    };

    const auto address = boost::asio::ip::address_v4::any();
    const auto port = static_cast<unsigned short>(8081);

    http_listener::launch(g_ioc, {address, port}, std::move(on_accept), std::move(on_error));
    sig_int_term.async_wait([](const boost::system::error_code &, int){
        g_ioc.stop();
    });

    size_t threads_num = 4;
    std::vector<std::thread> threads;
    for (size_t i = 0; i < threads_num; ++i)
        threads.emplace_back(std::bind(static_cast<std::size_t (boost::asio::io_context::*)()>(&boost::asio::io_context::run), std::ref(g_ioc)));

    for (auto &thr : threads)
        thr.join();

    return 0;
}
