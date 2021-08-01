#pragma once

#include <regex>

#include "base/lockable.hpp"

namespace beast_router {

template<class Session>
class router
{
public:
    using self_type = router<Session>;

    using session_type = Session;

    using mutex_type = typename base::lockable::mutex_type;

    using resource_map_type = typename session_type::resource_map_type;

    using method_type = typename session_type::method_type;

    using method_map_type = typename session_type::method_map_type;

    using storage_type = typename session_type::storage_type;

    using method_map_pointer = std::shared_ptr<method_map_type>;

    using method_const_map_pointer = std::shared_ptr<const method_map_type>;

    router();

    mutex_type &get_mutex() const;

    template<class ...OnRequest>
    auto get(const std::string &path, OnRequest &&...on_request)
        -> decltype(storage_type(std::declval<OnRequest>()...), void());

    template<class ...OnRequest>
    auto put(const std::string &path, OnRequest &&...on_request)
        -> decltype(storage_type(std::declval<OnRequest>()...), void());

    template<class ...OnRequest>
    auto post(const std::string &path, OnRequest &&...on_request)
        -> decltype(storage_type(std::declval<OnRequest>()...), void());

    template<class ...OnRequest>
    auto delete_(const std::string &path, OnRequest &&...on_request)
        -> decltype(storage_type(std::declval<OnRequest>()...), void());

    method_const_map_pointer get_resource_map() const;

private:
    void add_resource(const std::string &path, const method_type &method, storage_type &&storage);

    mutable mutex_type m_mutex;
    method_map_pointer m_method_map;
};

} // namespace beast_router

#include "impl/router.hpp"
