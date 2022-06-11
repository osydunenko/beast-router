#pragma once

namespace beast_router {
namespace mime_type {

static std::string_view get_mime_type(std::string_view ext) {
  std::string_view ret = default_mime;
  if (auto it = mime_types.find(ext); it != mime_types.end()) {
    ret = it->second;
  }
  return ret;
}

}  // namespace mime_type

template <class Body, class Version, class... Args,
          std::enable_if_t<std::is_convertible_v<Version, unsigned>, bool>>
static typename details::message_creator<false, Body>::return_type
create_response(http::status code, Version version, Args &&...args) {
  return details::message_creator<false, Body>::create(
      code, version, std::forward<Args>(args)...);
}

template <class Body, class Version, class... Args,
          std::enable_if_t<std::is_convertible_v<Version, unsigned>, bool>>
static typename details::message_creator<true, Body>::return_type
create_request(boost::beast::http::verb verb, Version version,
               std::string_view target, Args &&...args) {
  return details::message_creator<true, Body>::create(
      verb, version, target, std::forward<Args>(args)...);
}

template <class Version>
static http_empty_response make_empty_response(http::status code,
                                               Version version) {
  return create_response<http::empty_body>(code, version);
}

template <class Version>
static http_empty_response make_moved_response(Version version,
                                               std::string_view location) {
  auto rp = make_empty_response(http::status::moved_permanently, version);
  rp.set(http::field::location, location);
  return rp;
}

template <class Version>
static http_string_response make_string_response(http::status code,
                                                 Version version,
                                                 std::string_view data,
                                                 std::string_view content) {
  auto rp = create_response<http::string_body>(code, version, data);
  rp.prepare_payload();
  rp.set(http::field::content_type, content);
  return rp;
}

template <class Version>
static http_file_response make_file_response(Version version,
                                             const std::string &file_name) {
  boost::beast::error_code ec;
  http::file_body::value_type body;
  body.open(file_name.c_str(), boost::beast::file_mode::scan, ec);

  if (ec) {
    return create_response<http::file_body>(http::status::not_found, version);
  }

  const auto size = body.size();

  std::string file_extension;
  if (auto idx = file_name.rfind('.'); idx != std::string::npos) {
    file_extension = file_name.substr(idx + 1);
  }

  auto rp = create_response<http::file_body>(http::status::ok, version,
                                             std::move(body));
  rp.content_length(size);
  rp.set(http::field::content_type,
         mime_type::get_mime_type(std::move(file_extension)));
  return rp;
}

template <class Version>
static http_empty_request make_empty_request(boost::beast::http::verb verb,
                                             Version version,
                                             std::string_view target) {
  return create_request<http::empty_body>(verb, version, target);
}

}  // namespace beast_router
