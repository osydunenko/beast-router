#pragma once

#define CONNECTOR_TEMPLATE_DECLARE       \
    template<                            \
    class Protocol,                      \
    class Resolver,                      \
    class Socket,                        \
    template<typename> class Endpoint>

namespace beast_router {

namespace net = boost::asio;

CONNECTOR_TEMPLATE_DECLARE
connector<CONNECTOR_TEMPLATE_ATTRIBUTES>::connector(boost::asio::io_context &ctx, on_connect_type &&on_connect)
    : base::strand_stream{ctx.get_executor()}
    , m_resolver{ctx}
    , m_on_connect{std::move(on_connect)}
    , m_on_error{nullptr}
    , m_connection{socket_type{ctx}, static_cast<base::strand_stream &>(*this)}
{
}

CONNECTOR_TEMPLATE_DECLARE
connector<CONNECTOR_TEMPLATE_ATTRIBUTES>::connector(boost::asio::io_context &ctx, on_connect_type &&on_connect, on_error_type &&on_error)
    : base::strand_stream{ctx.get_executor()}
    , m_resolver{ctx}
    , m_on_connect{std::move(on_connect)}
    , m_on_error{std::move(on_error)}
    , m_connection{socket_type{ctx}, static_cast<base::strand_stream &>(*this)}
{
}

CONNECTOR_TEMPLATE_DECLARE
void connector<CONNECTOR_TEMPLATE_ATTRIBUTES>::do_resolve(std::string_view address, std::string_view port)
{
    m_resolver.async_resolve(address, port,
        boost::beast::bind_front_handler(
            &self_type::on_resolve,
            this->shared_from_this()
        ));
}

CONNECTOR_TEMPLATE_DECLARE
void connector<CONNECTOR_TEMPLATE_ATTRIBUTES>::on_resolve(boost::beast::error_code ec, results_type results)
{
    if (ec) {
        if (m_on_error) {
            m_on_error(ec, "on_resovle");
        }
        return;
    }

    m_connection.async_connect(std::move(results), 
        std::bind(&self_type::on_connect,
            this->shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2
        )
    );
}

CONNECTOR_TEMPLATE_DECLARE
void connector<CONNECTOR_TEMPLATE_ATTRIBUTES>::on_connect(boost::beast::error_code ec, typename results_type::endpoint_type ep)
{
    assert(m_on_connect != nullptr);
    boost::ignore_unused(ep);

    if (ec) {
        if (m_on_error) {
            m_on_error(ec, "on_connect");
        }
        return;
    }

    m_on_connect(m_connection.release());
}

} // namespace beast_router
