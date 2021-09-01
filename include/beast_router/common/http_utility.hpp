#pragma once

#include <map>
#include <tuple>
#include <string_view>

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/file_body.hpp>
#include <boost/beast/http/empty_body.hpp>

namespace beast_router {

namespace http = boost::beast::http;

/// Http response associated with the response body
template<class Body>
using beast_http_response = http::response<Body>;

/// String response
using http_string_response = beast_http_response<http::string_body>;

/// File resposne
using http_file_response = beast_http_response<http::file_body>;

/// Emtpy response
using http_empty_response = beast_http_response<http::empty_body>;

namespace details {

template<class Body>
struct response_creator
{
    using body_type = Body;

    using return_type = http::response<body_type>;

    template<class ...Args>
    inline static return_type
    create(http::status code, unsigned version, Args &&...args)
    {
        return return_type{
            std::piecewise_construct,
            std::forward_as_tuple(std::forward<Args>(args)...),
            std::forward_as_tuple(code, version)
        };
    }
};

template<>
struct response_creator<http::empty_body>
{
    using body_type = http::empty_body;

    using return_type = http::response<body_type>;

    template<class ...Args>
    inline static return_type
    create(http::status code, unsigned version, Args &&...)
    {
        return return_type{code, version};
    }
};

} // namespace details

/// The common struct to work with the mime types
struct mime_type
{
    /// The self type
    using self_type = mime_type;

    /// Map container type holds the mime types
    using container_type = std::map<std::string_view, std::string_view>;

    /// The defalt mime type
    static constexpr std::string_view default_mime = "application/text";

    /// Returns a mime type by the given extension
    /**
     * @param ext An extension for a type to be returned
     * @returns mime type
     */
    static std::string_view
    get(std::string_view ext);

private:
    static const container_type mime_types;
};

/// Creates a response associated with the Body type
/**
 * @param code HTTP code
 * @param version Version of HTTP to be sent
 * @param args A tuple forwarded as a parameter pack to the `Fields` constructor
 * @returns http response associated with the body
 */
template<
    class Body, 
    class Version,
    class ...Args,
    std::enable_if_t<
        std::is_convertible_v<Version, unsigned>, bool
    > = true>
inline typename details::response_creator<Body>::return_type
create_response(http::status code, Version version, Args &&...args);

/// Creates redirection response within the `Location`
/**
 * @param version HTTP version
 * @param location Redirection location
 * @returns response with the HTTP code - moved permanently
 */
template<class Version>
http_empty_response
make_moved_response(Version version, std::string_view location);

/// Creates string response by the given HTTP code and string data
/**
 * @param code HTTP status code
 * @param version HTTP version
 * @param data String data to be transmitted
 * @param content A mime type of the data
 * @returns string body response
 */
template<class Version>
http_string_response
make_string_response(http::status code, Version version, 
    std::string_view data, std::string_view content = "text/html");

/// Creates file response by the given file name
/**
 * @param version HTTP version
 * @param file file path
 * @returns file_name response
 *
 * @note For error handling the operates on the HTTP code.
 * If a file is not found or cannot be read then `not_found` is set;
 * otherwise the `ok` status code is sent
 */
template<class Version>
http_file_response
make_file_response(Version version, const std::string &file_name);

} // namespace beast_router

#include "impl/http_utility.ipp"
