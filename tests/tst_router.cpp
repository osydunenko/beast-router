#include <type_traits>
#include <memory>
#include <boost/test/unit_test.hpp>

#include "beast_router.hpp"
#include "beast_router/base/strand_stream.hpp"

namespace utf = boost::unit_test_framework;
namespace net = boost::asio::ip;

struct tst_session
{
    struct impl;

    using timer_type = beast_router::timer<beast_router::base::strand_stream::asio_type, boost::asio::steady_timer>;

    using timer_duration_type = typename timer_type::duration_type;

    using is_request = std::true_type;

    using body_type = beast_router::http::empty_body;

    using message_type = beast_router::http::request<body_type>;

    using method_type = beast_router::http::verb;

    using impl_type = impl;

    using context_type = beast_router::http_server::session_type::context<impl_type>;

    using router_type = beast_router::router<tst_session>;

    struct impl: public beast_router::base::strand_stream,
        public std::enable_shared_from_this<impl> 
    {
        using self_type = impl;

        impl(net::tcp::socket &&socket)
            : beast_router::base::strand_stream{socket.get_executor()}
            , m_socket{std::move(socket)}
        {}

        template<class Message>
        self_type &send(Message &&message, timer_duration_type duration)
        {
            boost::ignore_unused(message);
            boost::ignore_unused(duration);
            return *this;
        }

        net::tcp::socket m_socket;
    };
};

using message_type = typename tst_session::message_type;
using context_type = typename tst_session::context_type;

beast_router::router<tst_session> router;
beast_router::base::dispatcher<tst_session> dispatcher{router};

boost::asio::io_context ioc;
unsigned num_single_requests = 0;

BOOST_AUTO_TEST_CASE(test_single_request, * utf::depends_on("single_request"))
{
    std::shared_ptr<tst_session::impl_type> impl{new tst_session::impl_type(net::tcp::socket{ioc})};

    dispatcher.do_process_request(
        beast_router::make_empty_request(beast_router::http::verb::get, 11, "single_request"), *impl);

    dispatcher.do_process_request(
        beast_router::make_empty_request(beast_router::http::verb::put, 11, "single_request"), *impl);

    dispatcher.do_process_request(
        beast_router::make_empty_request(beast_router::http::verb::post, 11, "single_request"), *impl);

    dispatcher.do_process_request(
        beast_router::make_empty_request(beast_router::http::verb::delete_, 11, "single_request"), *impl);

    dispatcher.do_process_request(
        beast_router::make_empty_request(beast_router::http::verb::get, 11, "not_found"), *impl);

    BOOST_CHECK_EQUAL(num_single_requests, 31);
}

BOOST_AUTO_TEST_CASE(single_request)
{
    router.get(R"(^single_request$)", [](const message_type &msg, context_type &ctx) {
        boost::ignore_unused(ctx);
        BOOST_CHECK_EQUAL(msg.target(), "single_request");
        BOOST_TEST(!(num_single_requests & 1u));
        num_single_requests |= 1u;
    });

    router.put(R"(^single_request$)", [](const message_type &msg, context_type &ctx) {
        boost::ignore_unused(ctx);
        BOOST_CHECK_EQUAL(msg.target(), "single_request");
        BOOST_TEST(!(num_single_requests & (1u << 1)));
        num_single_requests |= 1u << 1;
    });

    router.post(R"(^single_request$)", [](const message_type &msg, context_type &ctx) {
        boost::ignore_unused(ctx);
        BOOST_CHECK_EQUAL(msg.target(), "single_request");
        BOOST_TEST(!(num_single_requests & (1u << 2)));
        num_single_requests |= 1u << 2;
    });

    router.delete_(R"(^single_request$)", [](const message_type &msg, context_type &ctx) {
        boost::ignore_unused(ctx);
        BOOST_CHECK_EQUAL(msg.target(), "single_request");
        BOOST_TEST(!(num_single_requests & (1u << 3)));
        num_single_requests |= 1u << 3;
    });

    router.not_found([](context_type &ctx) {
        boost::ignore_unused(ctx);
        BOOST_TEST(!(num_single_requests & (1u << 4)));
        num_single_requests |= 1u << 4;
    });
}