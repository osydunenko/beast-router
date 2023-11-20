#pragma once

#define CONNECTION_TEMPLATE_DECLARE \
    template <class Stream, class CompletionExecutor>

ROUTER_NAMESPACE_BEGIN()

CONNECTION_TEMPLATE_DECLARE
connection<CONNECTION_TEMPLATE_ATTRIBUTES>::connection(
    Stream&& stream, const CompletionExecutor& executor)
    : base::connection<connection<Stream, CompletionExecutor>,
        CompletionExecutor> { executor }
    , m_stream { std::move(stream) }
{
}

CONNECTION_TEMPLATE_DECLARE
connection<CONNECTION_TEMPLATE_ATTRIBUTES>::~connection()
{
    if (is_open()) {
        shutdown(shutdown_type::shutdown_both);
    }
}

CONNECTION_TEMPLATE_DECLARE
ROUTER_DECL boost::beast::error_code
connection<CONNECTION_TEMPLATE_ATTRIBUTES>::shutdown(shutdown_type type)
{
    auto ec = boost::system::error_code {};
    m_stream.shutdown(type, ec);

    return ec;
}

CONNECTION_TEMPLATE_DECLARE
ROUTER_DECL boost::beast::error_code
connection<CONNECTION_TEMPLATE_ATTRIBUTES>::close()
{
    auto ec = boost::system::error_code {};
    m_stream.close(ec);

    return ec;
}

CONNECTION_TEMPLATE_DECLARE
ROUTER_DECL bool connection<CONNECTION_TEMPLATE_ATTRIBUTES>::is_open() const
{
    return m_stream.is_open();
}

CONNECTION_TEMPLATE_DECLARE
ROUTER_DECL typename connection<CONNECTION_TEMPLATE_ATTRIBUTES>::stream_type
connection<CONNECTION_TEMPLATE_ATTRIBUTES>::release()
{
    // making an rvalue reference since the copy is forbidden
    return std::move(m_stream);
}

CONNECTION_TEMPLATE_DECLARE
ROUTER_DECL typename connection<CONNECTION_TEMPLATE_ATTRIBUTES>::stream_type&
connection<CONNECTION_TEMPLATE_ATTRIBUTES>::stream()
{
    return m_stream;
}

ROUTER_NAMESPACE_END()
