#pragma once

#include "../base/connection.hpp"

#define CONNECTION_TEMPLATE_ATTRIBUTES Stream, CompletionExecutor

namespace beast_router {

/// Encapsulates the connections related functionality
template <class Stream, class CompletionExecutor>
class connection
    : public base::connection<connection<Stream, CompletionExecutor>,
          CompletionExecutor> {
public:
    /// The self type
    using self_type = connection<CONNECTION_TEMPLATE_ATTRIBUTES>;

    /// The stream type
    using stream_type = Stream;

    /// The shutdown type
    using shutdown_type = typename stream_type::shutdown_type;

    /// Constructor
    explicit connection(Stream&& stream, const CompletionExecutor& executor);

    /// Destructor
    ~connection() override;

    /// Shutes down the connection
    /**
     * @param type The shutdown parameter
     * @returns error_code
     */
    ROUTER_DECL boost::beast::error_code shutdown(shutdown_type type);

    /// Closes the connection
    /**
     * @returns error_code
     */
    ROUTER_DECL boost::beast::error_code close();

    /// Obtains the connection state
    /**
     * @returns bool
     */
    ROUTER_DECL bool is_open() const;

    /// Returns the stream and releases the ownership
    /**
     * @returns stream_type
     */
    ROUTER_DECL stream_type release();

    /// Obtains the reference to the stream
    /**
     * @returns stream_type
     */
    ROUTER_DECL stream_type& stream();

private:
    stream_type m_stream;
};

} // namespace beast_router

#include "impl/connection.ipp"
