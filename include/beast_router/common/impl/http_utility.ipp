#pragma once

namespace beast_router {

const mime_type::container_type mime_type::mime_types = {
    {"html",    "text/html"},
    {"js",      "application/javascript"}
};

std::string_view mime_type::get(std::string_view ext)
{
    std::string_view ret = mime_type::default_mime;
    const mime_type::container_type &types = mime_type::mime_types;
    if (auto it = types.find(ext); it != types.end()) {
        ret = it->second;
    }

    return ret;
}

template<
    class Body, 
    class Version,
    class ...Args,
    std::enable_if_t<std::is_convertible_v<Version, unsigned>, bool> = true>
typename details::response_creator<Body>::return_type
create_response(http::status code, Version version, Args &&...args)
{
    return details::response_creator<Body>::create(code,
        version, std::forward<Args>(args)...);
}

template<class Version>
http_empty_response make_moved_response(Version version, std::string_view location)
{
    auto rp = create_response<http::empty_body>(
        http::status::moved_permanently, version);
    rp.set(http::field::location, location);
    return rp;
}

template<class Version>
http_string_response make_string_response(http::status code, 
    Version version, std::string_view data, std::string_view content)
{
    auto rp = create_response<http::string_body>(code, version, data);
    rp.prepare_payload();
    rp.set(http::field::content_type, content);
    return rp;
}

template<class Version>
http_file_response
make_file_response(Version version, const std::string &file_name)
{
    boost::beast::error_code ec;
    http::file_body::value_type body;
    body.open(file_name.c_str(), boost::beast::file_mode::scan, ec);

    if (ec) {
        return create_response<http::file_body>(
            http::status::not_found, version);
    }

    const auto size = body.size();

    std::string file_extension;
    if (auto idx = file_name.rfind('.'); idx != std::string::npos) {
        file_extension = file_name.substr(idx + 1);
    }

    auto rp = create_response<http::file_body>(
        http::status::ok, version, std::move(body));
    rp.content_length(size);
    rp.set(http::field::content_type, 
        mime_type::get(std::move(file_extension)));
    return rp;
}

} // namespace beast_router
