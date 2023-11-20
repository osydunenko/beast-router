#pragma once

#include "../../common/utility.hpp"

#define BASE_CONNECTION_TEMPLATE_DECLARE \
    template <class Derived, class CompletionExecutor>

ROUTER_NAMESPACE_BEGIN()
namespace base {

BASE_CONNECTION_TEMPLATE_DECLARE
connection<BASE_CONNECTION_TEMPLATE_ATTRIBUTES>::connection(
    const CompletionExecutor& executor)
    : m_completion_executor { executor }
{
}

BASE_CONNECTION_TEMPLATE_DECLARE
template <class Function, class EndpointSequence>
void connection<BASE_CONNECTION_TEMPLATE_ATTRIBUTES>::async_connect(
    const EndpointSequence& endpoints, Function&& func)
{
    static_assert(
        utility::details::is_all_true<
            std::is_invocable_v<
                Function, boost::system::error_code,
                typename std::decay_t<
                    decltype(std::declval<Derived>().stream())>::endpoint_type>,
            boost::asio::is_endpoint_sequence<EndpointSequence>::value>::value,
        "connection::async_connect requirements are not met");

    boost::asio::async_connect(
        derived().stream(), endpoints,
        boost::asio::bind_executor(m_completion_executor,
            std::forward<Function>(func)));
}

BASE_CONNECTION_TEMPLATE_DECLARE
template <class Function, class Serializer>
void connection<BASE_CONNECTION_TEMPLATE_ATTRIBUTES>::async_write(
    Serializer& serializer, Function&& func)
{
    static_assert(
        std::is_invocable_v<Function, boost::system::error_code, size_t>,
        "connection::async_write/Function requirements are not met");

    boost::beast::http::async_write(
        derived().stream(), serializer,
        boost::asio::bind_executor(m_completion_executor,
            std::forward<Function>(func)));
}

BASE_CONNECTION_TEMPLATE_DECLARE
template <class Function, class Buffer, class Parser>
void connection<BASE_CONNECTION_TEMPLATE_ATTRIBUTES>::async_read(
    Buffer& buffer, Parser& parser, Function&& func)
{
    static_assert(
        utility::is_all_true_v<
            std::is_invocable_v<Function, boost::system::error_code, size_t>,
            boost::asio::is_dynamic_buffer<Buffer>::value>,
        "connection::async_read requirements are not met");

    boost::beast::http::async_read(
        derived().stream(), buffer, parser,
        boost::asio::bind_executor(m_completion_executor,
            std::forward<Function>(func)));
}

} // namespace base
ROUTER_NAMESPACE_END()