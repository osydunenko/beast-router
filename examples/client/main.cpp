#include <iostream>
#include "beast_router.hpp"

using namespace std;
using namespace boost;
using namespace beast_router;

static asio::io_context ioc;

static asio::signal_set sig_int_term{ioc, SIGINT, SIGTERM};

int main(int, char **)
{
    plain_connector::on_connect_type on_connect = [](plain_connector::socket_type socket) 
    {
        http::request<http::empty_body> req{http::verb::get, "/", 11};
        req.set(http::field::host, "wttr.in");
        req.set(http::field::user_agent, "curl");

        client_session::router_type router;
        router.handle_response([](const client_session::message_type &rp, client_session::context_type &) {
            cout << rp << endl;
            ioc.stop();
        });

        plain_connector::on_error_type on_error = [](system::error_code, std::string_view msg)
        {
            cerr << "Error: " << msg << endl;
            ioc.stop();
        };

        client_session::send(std::move(socket), std::move(req), router, std::move(on_error)).recv();
    };

    plain_connector::on_error_type on_error = [](system::error_code, std::string_view msg)
    {
        cerr << "Error: " << msg << endl;
        ioc.stop();
    };

    plain_connector::connect(ioc, {"wttr.in"}, {"http"}, std::move(on_connect), std::move(on_error));

    sig_int_term.async_wait([](const system::error_code &, int) {
        ioc.stop();
    });

    ioc.run();

    return 0;
}
