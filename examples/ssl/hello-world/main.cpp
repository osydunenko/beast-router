#include "beast_router.hpp"
#include <atomic>
#include <exception>
#include <iostream>
#include <sstream>
#include <string_view>

using namespace std::chrono_literals;

/// requests counter
static std::atomic<uint64_t> counter { 0 };

/// ip address
static const auto address = boost::asio::ip::address_v4::any();

/// port to start listening on
static const auto port = static_cast<unsigned short>(8080);

/// SSL Context
boost::asio::ssl::context ctx { boost::asio::ssl::context::sslv23_server };

/// Declare CA certificate
static const std::string_view ca_cert = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDazCCAlOgAwIBAgIUdnC0zJGGH5SSCEhn5Am426LdtugwDQYJKoZIhvcNAQEL\n"
    "BQAwRTELMAkGA1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoM\n"
    "GEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZDAeFw0yMzExMjMxMzE4MjlaFw0yMzEy\n"
    "MjMxMzE4MjlaMEUxCzAJBgNVBAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEw\n"
    "HwYDVQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQwggEiMA0GCSqGSIb3DQEB\n"
    "AQUAA4IBDwAwggEKAoIBAQCyfUR8po/n7+47hIf7MY23eDv71SUibE5iDycJvVXz\n"
    "wHKVPP8erZ2DUoNgYa+9dHkV9vPlV3w9gwas/PlBWctAdE4yTtowqCHsvhrKCyP2\n"
    "2cKJZ4DqrPBjjSJcyydAgn53GPcwIVqRyolpzJp+qFtiGq/zxd8Rk/FqcSkrWh2U\n"
    "OEK1f7xN5tUZ0VzPzoj7BgYxDo7UIfXtG5VxDljS56PC7IKx/g6PnDeG/rhDTMXX\n"
    "k2/5XrtR98C1Jz1UFw+HeZoAP0RM1pvW+f1FwuMl2zl3LUG4EYqJfYpxs/wLI95/\n"
    "VxM+iDumJ8Ry4eyP7qrz+fYYyDYftAdcdkjmpq+qziyDAgMBAAGjUzBRMB0GA1Ud\n"
    "DgQWBBTSOmCJAOVF4DBsTouZ0Xv/N0yedDAfBgNVHSMEGDAWgBTSOmCJAOVF4DBs\n"
    "TouZ0Xv/N0yedDAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQAk\n"
    "Y3XNcfU9jlwz33PxAmsUhr+kfH7fQ7EnrcUAzOGJ5tkWx+icXtWPthzip1UTp/FK\n"
    "9/HIcFTq6D4bP8j2ULMe/308+41inOCBRiMTul7sYR4Z2Le5kyzKXlWkbw3rjuS5\n"
    "8tx7Tab45ulXVd9geZI/55fmjFvGftRlHItdjH/ZvHyeTgAgtSbfVv+Ty6Ia7N92\n"
    "x+t5lEyyiP7/Rls9830Q/LkoIW11L7XqDqLn+u1YzbyEASuL/CXDP3syIRO9zNkq\n"
    "9gm1XJHb5rMn1SDJEpjxD28WtSaH67Gmd/XHDwCwwrDKuRTrsK/MUywh1eKB/hpj\n"
    "ROHtZA681nCoDpuBMp63\n"
    "-----END CERTIFICATE-----\n"
};

/// Declare Key content
static const std::string_view ca_key = {
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEogIBAAKCAQEAsn1EfKaP5+/uO4SH+zGNt3g7+9UlImxOYg8nCb1V88BylTz/\n"
    "Hq2dg1KDYGGvvXR5Ffbz5Vd8PYMGrPz5QVnLQHROMk7aMKgh7L4aygsj9tnCiWeA\n"
    "6qzwY40iXMsnQIJ+dxj3MCFakcqJacyafqhbYhqv88XfEZPxanEpK1odlDhCtX+8\n"
    "TebVGdFcz86I+wYGMQ6O1CH17RuVcQ5Y0uejwuyCsf4Oj5w3hv64Q0zF15Nv+V67\n"
    "UffAtSc9VBcPh3maAD9ETNab1vn9RcLjJds5dy1BuBGKiX2KcbP8CyPef1cTPog7\n"
    "pifEcuHsj+6q8/n2GMg2H7QHXHZI5qavqs4sgwIDAQABAoIBAFyTrJoaqjlasO4d\n"
    "54naQe8PZc1Q2FnqYx1pTo42rgYno0bUxF5dHn2mpo2vHT/e0Y8a75XcsowVEblX\n"
    "3NCQimN777MYQwNJsY7ha3OwI489kzFBhhQybtyzr0cB9/H1vTJ4uH02T4ueyXce\n"
    "sGNRX1SbEvgVgYXUfjr/RqM9smnVhjlaF0QeU9N0KA9/NbdQ2jqj0qQoXppo334Z\n"
    "E8AsXK7xsZeJNUwQBkZ+zzjS6V1WndMEYNFx6yppuznBb4KfmsIoDWTNJ9ZQBrof\n"
    "19ukowoIzh1kTsdKBC60XjrdHG5N9fuifZpS/WdpiutotvPIA1ZJ6Us7zZJ0Y5be\n"
    "6EB8WoECgYEA5VJ3P6xW5LAZqEYxOJ6r/BKAoIILhF1OTsLRVyAA5IRzbAGXl0pL\n"
    "EX0bTibIJyI7LEob1F0PglZvjmTFaTqn1CZ9HFBE8XMfYbY8E30i4spxSc/LNWd4\n"
    "Ub2kwROWvatJQqKi+LnvstkfLvbMfCLDGU0B0G46J71xrNM9QM9zFUkCgYEAx0Dt\n"
    "+PByatOTeAUCy4WSuP9Ms4Ru/VHPIScnmqmura/BFSxuzd6sDCR69YgMGsKZjQ5g\n"
    "8BIbipz6hI2elCJpFDjPmGAkZfsUxUweQ26qfbn1zoeOXAjUg4cb91AIV/26hI/m\n"
    "/9Qszdy0r1eOAvtlj/2fPPxywg1CgCfgu8zND2sCgYBcH/z3/2wJAxXLnCc578R3\n"
    "x5cU5ClsS2+iBHHE5n51TyBvS1Ry2s29gNzvUHUoA4ByEnOLpLcOTVsTgTgtRfsW\n"
    "J0Arl7Oaq/z3bBZGXgcdxOYuGOQx2Bdl/yGozw3HtIAB3QRLl6bL2p3EaDFNzUlD\n"
    "aMRJz35daKW6IEKDPtOkwQKBgBe6ztyP3kCEtBJeHmgYn1Gy7fKPOhynKpDbNedA\n"
    "gBIlVUxtP0D7XOgRTCeDrVVeiaT36mmM7oTCjz9MEm+37WXAIlEWWh9fGKkqmIwV\n"
    "WO6iP/j5weWKE60aYSVB/cxk5lq1PKCJJ1DZERe0yK/oOr88SEOeGRitNZdHqIcV\n"
    "K/LDAoGAMp/Td9Zoj4H22CYhJjDOg5VWeAtR9wHi1tKdZ2HmeNOqqjUVjE3/at5K\n"
    "YokdPDRXuatl8grDkqJeyCHP5b6HrkdXWk2DxPq9mpTpfA6nrL5c5W4Jwp7VrjHF\n"
    "2PyfPp8s0xqHZ8Q1vYakzTvYpX+mcOmqYWH9JdtXJkD9/u8nM+0=\n"
    "-----END RSA PRIVATE KEY-----\n"
};

int main(int, char**)
{
    /// Prepare ssl context
    ctx.set_options(boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::sslv23
        | boost::asio::ssl::context::no_sslv2);
    ctx.use_certificate_chain({ ca_cert.data(), ca_cert.size() });
    ctx.use_private_key({ ca_key.data(), ca_key.size() },
        boost::asio::ssl::context::pem);

    /// routing table
    beast_router::ssl::http_server_type::router_type router {};

    /// Add the callback and link to index ("/") requests patterns
    router.get(R"(^/$)", [](const beast_router::ssl::http_server_type::message_type& rq, beast_router::ssl::http_server_type::context_type& ctx) {
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
                                                                   std::string_view msg) {
        throw std::runtime_error(ec.message());
    };

    beast_router::http_listener_type::on_accept_type on_accept = [&router, ssl_ctx = std::ref(ctx), &on_error](beast_router::http_listener_type::socket_type socket) {
        beast_router::ssl::http_server_type::recv(std::piecewise_construct, ssl_ctx, std::move(socket), router,
            1s, on_error);
    };

    /// Start listening
    beast_router::http_listener_type::launch(*event_loop, { address, port }, std::move(on_accept), std::move(on_error));

    /// Start serving
    return event_loop->exec();
}
