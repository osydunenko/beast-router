#pragma once

#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/file_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <map>
#include <string_view>
#include <tuple>

ROUTER_NAMESPACE_BEGIN()

namespace http = boost::beast::http;

/// Http response associated with the response body
template <class Body>
using beast_http_response = http::response<Body>;

/// Http request associated with the request body
template <class Body>
using beast_http_request = http::request<Body>;

/// String response
using http_string_response = beast_http_response<http::string_body>;

/// File resposne
using http_file_response = beast_http_response<http::file_body>;

/// Emtpy body response
using http_empty_response = beast_http_response<http::empty_body>;

/// Empty body request
using http_empty_request = beast_http_request<http::empty_body>;

namespace details {

template <bool IsRequest, class Body>
struct message_creator {
    using return_type = std::conditional_t<IsRequest, http::request<Body>, http::response<Body>>;

    template <class... Args>
    static return_type create(http::status code, unsigned version,
        Args&&... args)
    {
        return return_type { std::piecewise_construct,
            std::forward_as_tuple(std::forward<Args>(args)...),
            std::forward_as_tuple(code, version) };
    }

    static return_type create(boost::beast::http::verb verb, unsigned version,
        std::string_view target)
    {
        return return_type { verb, target, version };
    }
};

template <bool IsRequest>
struct message_creator<IsRequest, http::empty_body> {
    using return_type = std::conditional_t<IsRequest, http::request<http::empty_body>,
        http::response<http::empty_body>>;

    static return_type create(http::status code, unsigned version)
    {
        return return_type { code, version };
    }

    static return_type create(boost::beast::http::verb verb, unsigned version,
        std::string_view target)
    {
        return return_type { verb, target, version };
    }
};

} // namespace details

namespace mime_type {

/// Mime types container type
using container_type = std::map<std::string_view, std::string_view>;

/// Default mime type
static constexpr std::string_view default_mime = "application/type";

/// Mime types list
static const container_type mime_types {
    { "html", "text/html" }, { "js", "application/javascript" },
    { "css", "text/css" }, { "png", "image/png" },
    { "jpg", "image/jpeg" }, { "fvl", "video/x-flv" }
};

/// Returns a mime type by the given extension
/**
 * @param ext An extension for a type to be returned
 * @returns mime type
 */
static std::string_view get_mime_type(std::string_view ext);

} // namespace mime_type

/// Creates a response associated with the Body type
/**
 * @param code HTTP code
 * @param version Version of HTTP to be sent
 * @param args A tuple forwarded as a parameter pack to the `Fields` constructor
 * @returns http response associated with the body
 */
template <
    class Body, class Version, class... Args,
    std::enable_if_t<std::is_convertible_v<Version, unsigned>, bool> = true>
static typename details::message_creator<false, Body>::return_type
create_response(http::status code, Version version, Args&&... args);

/// Creates a request associated with the Body type
/**
 * @param verb HTTP method
 * @param version Version of HTTP to be sent
 * @param target Target string given as a resource request
 * @param args A tuple forwarded as a parameter pack to the `Fields` constructor
 * @returns http request associated with the body
 */
template <
    class Body, class Version, class... Args,
    std::enable_if_t<std::is_convertible_v<Version, unsigned>, bool> = true>
static typename details::message_creator<true, Body>::return_type
create_request(boost::beast::http::verb verb, Version version,
    std::string_view target, Args&&... args);

/// Creates empty response
/**
 * @param code HTTP status code
 * @param version HTTP version
 * @returns http_empty_response with the HTTP code
 */
template <class Version>
static http_empty_response make_empty_response(http::status code,
    Version version);

/// Creates redirection response within the `Location`
/**
 * @param version HTTP version
 * @param location Redirection location
 * @returns http_empty_response with the HTTP code - moved permanently
 */
template <class Version>
static http_empty_response make_moved_response(Version version,
    std::string_view location);

/// Creates string response by the given HTTP code and string data
/**
 * @param code HTTP status code
 * @param version HTTP version
 * @param data String data to be transmitted
 * @param content A mime type of the data
 * @returns http_string_response
 */
template <class Version>
static http_string_response make_string_response(
    http::status code, Version version, std::string_view data,
    std::string_view content = "text/html");

/// Creates file response by the given file name
/**
 * @param version HTTP version
 * @param file file path
 * @returns file_name_response
 *
 * @note For error handling the operates on the HTTP code.
 * If a file is not found or cannot be read then `not_found` is set;
 * otherwise the `ok` status code is sent
 */
template <class Version>
static http_file_response make_file_response(Version version,
    const std::string& file_name);

/// Creates empty request type
/**
 * @param ver HTTP method
 * @param version HTTP version
 * @param target Target string given as a resource request
 * @returns http_empty_response
 */
template <class Version>
static http_empty_request make_empty_request(boost::beast::http::verb verb,
    Version version,
    std::string_view target);

ROUTER_NAMESPACE_END()

#include "impl/http_utility.ipp"
