#include "beast_router.hpp"
#include <atomic>
#include <sstream>
#include <string_view>

/// requests counter
static std::atomic<uint64_t> counter { 0 };

/// SSL Context
boost::asio::ssl::context ctx { boost::asio::ssl::context::tlsv12 };

static const std::string_view crt {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDCTCCAfGgAwIBAgIUQYN2nva8NiEaXPsuSvIYI3szEcowDQYJKoZIhvcNAQEL\n"
    "BQAwFDESMBAGA1UEAwwJbG9jYWxob3N0MB4XDTIzMDQwNTExMTAzN1oXDTI3MDQw\n"
    "NTExMTAzN1owFDESMBAGA1UEAwwJbG9jYWxob3N0MIIBIjANBgkqhkiG9w0BAQEF\n"
    "AAOCAQ8AMIIBCgKCAQEAqXNjFyU8CcMe/5QATqNj0XcBy+Wq0jMEXdpPUBs3Ra8e\n"
    "J8Xc8o4j2agP3UzPAJYj6R6Iu+9pMa953wscnSVyjj0c38m+u3+KhZQjLe4OW463\n"
    "VxkNICJ6THZNLpGyuA7zGa1me2Iixi9FNpTC0LYm81AivfuCXKnQsdmpR4gd4lVP\n"
    "0U8x3h/+5XIQkJzLb9t3cFtv1koyf8SfWWJ/q7gJ/Ybtp/IME/2KPKdQwvhDIqMR\n"
    "/axGXR3tv/oMHiC3pmfXN4sZylgzF4KpzWnI0y4pRzKEZTEsPELFEn3fFCWqMtaM\n"
    "DUVNWkQ9OWKhuIBG67emikVxQkQ6BYS2UrQFlSjGZwIDAQABo1MwUTAdBgNVHQ4E\n"
    "FgQUy/C4lnRqzXm+96mOabNCw5Hwir0wHwYDVR0jBBgwFoAUy/C4lnRqzXm+96mO\n"
    "abNCw5Hwir0wDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAULY5\n"
    "Yv5tI1klCLyC/uXLxUVrdaN/87vpxxveDLll2PkGZd8eVkAVht8GbkpLIBZ7nwjc\n"
    "NYdZpXb5lFGQ9JSoVcX7FtmPAC9Mh9mmrL2AbHyMSjGppuO7MhMbOYp/lA2XUUhA\n"
    "jvQmw34qGTyAMX7eOmiJWG17vwOu8Y1TFLGoVBmf8+DYPCR/0dIXAnIBQHdSUFmT\n"
    "ymYouMNF6Kzqlhx0qwyboo4RJmkLlXHCEzbzUs1zsXNRvHocbUc2h9dBTknVB4E7\n"
    "db9Xw/yL+I34QqP7RNjk4yyORJewA3NEG6FNSya26VJGVbdPtJefEFWMgh/nDoub\n"
    "yjRQO4WksELUf4MXUg==\n"
    "-----END CERTIFICATE-----\n"
};

static const std::string_view key {
    "-----BEGIN PRIVATE KEY-----\n"
    "MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQCpc2MXJTwJwx7/\n"
    "lABOo2PRdwHL5arSMwRd2k9QGzdFrx4nxdzyjiPZqA/dTM8AliPpHoi772kxr3nf\n"
    "CxydJXKOPRzfyb67f4qFlCMt7g5bjrdXGQ0gInpMdk0ukbK4DvMZrWZ7YiLGL0U2\n"
    "lMLQtibzUCK9+4JcqdCx2alHiB3iVU/RTzHeH/7lchCQnMtv23dwW2/WSjJ/xJ9Z\n"
    "Yn+ruAn9hu2n8gwT/Yo8p1DC+EMioxH9rEZdHe2/+gweILemZ9c3ixnKWDMXgqnN\n"
    "acjTLilHMoRlMSw8QsUSfd8UJaoy1owNRU1aRD05YqG4gEbrt6aKRXFCRDoFhLZS\n"
    "tAWVKMZnAgMBAAECggEADBu1qfK8Loplzad6uiPMvyv80eAQa8K/fiFaZ4P6WB1i\n"
    "Tz9BQYfMogHzWEHjzMvttvu6k4tQ/f7m+3wkyEnjvKw7QvZ8jZNYh6EFPoPzPLGf\n"
    "AjdFC9XK3WnarAg3OBXBY0VTvF9P0s+P542LujQ55ksEoIS3VP3BbuP5x6W/VOvb\n"
    "rUjcYxUFr9OK8LShmnmisrE+ntRqlnqr4tmOZ1s5Qmw5Hy5eV7i2Ie0fLc+ingmF\n"
    "19j+E1mIo66Cv+7UeqQSRHkk1XpiehgtmExqRp4WsfVT37r18FSlSx1wTmrwczsf\n"
    "H9wxYgGszQk2HtyOfW/CqXzn+NmndPXl+GGRQoDxuQKBgQC3XBjr7gMCKoRmY2DJ\n"
    "NmKpoODVqHdwIXq0HByx8BJq3AFHBYd+iDgOaNQmfozhPZFoGUxf0VSCkLZZS2PB\n"
    "8ajGGsw1Q7fs1mJTO9349EzZEPotSfHdyk3YhJfFRRbiSYBxB2xMlp4Mw6nD9cOt\n"
    "IYdmHGvhsCun8zW1l2hsyGCZ5QKBgQDslKuMn7tr38K1zpOfAA9KGkPPXWFqCLj6\n"
    "2GPbIYJ+E9RJBkOMLKZjB2pdcCmKlYXHxidFX11mTr5mUkvjbAK/hWiBAAbido29\n"
    "zmN97L7NVtq09udQA7fwAuX37IJaBh0t1DGaXUHHWnF20iuNmYWwXwEdHSGu2Mff\n"
    "bMQLmCCqWwKBgF3G3oAbtLIw6JItFV0TUZaLzzG2/Y79sHHZRtvCesjoSEb4jvmp\n"
    "1XGZL5eYdZjlEi75cVQ4DU7RkFFO+3A/lh/rqLE9Nx4L7zG+lqIy3/LMegcboHXc\n"
    "d7/a4Hxl/3QwP16Pe1YYWjERCQxN74vmcAdLVemRXmKBQuDi1Od9+9n5AoGAAqkV\n"
    "WMp/EBJ/HQ5KqLIWee3br1xMeSXJ9sAyN0ekMQjGDWAtqEjkQh7WOmDFhtJxo7J9\n"
    "xJDy+vCNwZbRVahkS4UTjMfUS/2rUGQeyE6+Qo7kfL5+EW9JRUCzF1uoh5yj/Vzy\n"
    "hdrgn35L4lswtDHyx+35lDs8oru7W67ccYjvbRsCgYBMxuAvIX5x8O8ASu9jYmeD\n"
    "stDQ9sAN9ro8VFLnISFu1RFOZT4PvtE7g4QJ3dxFteOjVL6IsKq1AQa6uGwqhSSc\n"
    "oK5vEFDz7pMPYY3cXbTvkUcbo+1+fBFpEI7cTlVrXPCtD4DU79Cfo8T34+Kd742c\n"
    "UIWzbbHq3NLdPR14/h44Zg==\n"
    "-----END PRIVATE KEY-----\n"
};

static const std::string_view dh {
    "-----BEGIN DH PARAMETERS-----\n"
    "MIIBCAKCAQEAlXNFQg0bN3WPxrDTMaPwAX8aZb1lkMQvrB/C1t4Rc7Two1//+y2g\n"
    "UDN1cowlXqcNNGBZR+hqpPNNhsodK3pY1IA5MuCCj6qJV72cpTXJtucJASuYf5IB\n"
    "+IDA6ICLRsQXOuVdXvGHEorL5QZdIqxrFFynrU0P7JpfbhzbHV8eLa3lmclxmnyY\n"
    "4ji20eYQhnZ3CFKOtFGZ8rUnFKhJWVnwT8Q3ult6m19L0/q1JFQPnXbJQtrHUEaY\n"
    "vUaqoqEiZJaKG5ZnLaRJQiz2XPLKuelcVb6SJvXutcztzIVVKHESZiKA+t/9KZV3\n"
    "jCk7TXj7aTxTKQB+3A8OxRMM0etrCQM7NwIBAg==\n"
    "-----END DH PARAMETERS-----\n"
};

int main(int, char**)
{

    ctx.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::single_dh_use);
    ctx.use_certificate_chain({ crt.data(), crt.size() });
    ctx.use_private_key({ key.data(), key.size() }, boost::asio::ssl::context::file_format::pem);
    return 0;
    /* return beast_router::http_server()
        .on_get(R"(^.*$)", [](const auto& rq, auto& ctx) {
            std::stringstream i_str;
            i_str << "Hello World: the request was triggered [" << ++counter
                  << "] times";

            auto rp = beast_router::make_string_response(beast_router::http::status::ok, rq.version(), i_str.str());
            rp.keep_alive(rq.keep_alive());

            ctx.send(std::move(rp));
        })
        .exec(); */
    return 0;
}
