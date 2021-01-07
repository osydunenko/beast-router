#pragma once

#include <regex>

#include "base/lockable.h"

namespace server {

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

template<class Session>
router<Session>::router()
    : m_method_map{nullptr}
{
}

template<class Session>
template<class ...OnRequest>
auto router<Session>::get(const std::string &path, OnRequest &&...on_request)
    -> decltype(storage_type(std::declval<OnRequest>()...), void())
{
    add_resource(path, method_type::get, storage_type{std::forward<OnRequest>(on_request)...});
}

template<class Session>
template<class ...OnRequest>
auto router<Session>::put(const std::string &path, OnRequest &&...on_request)
    -> decltype(storage_type(std::declval<OnRequest>()...), void())
{
    add_resource(path, method_type::put, storage_type{std::forward<OnRequest>(on_request)...});
}

template<class Session>
template<class ...OnRequest>
auto router<Session>::post(const std::string &path, OnRequest &&...on_request)
    -> decltype(storage_type(std::declval<OnRequest>()...), void())
{
    add_resource(path, method_type::post, storage_type{std::forward<OnRequest>(on_request)...});
}

template<class Session>
template<class ...OnRequest>
auto router<Session>::delete_(const std::string &path, OnRequest &&...on_request)
    -> decltype(storage_type(std::declval<OnRequest>()...), void())
{
    add_resource(path, method_type::delete_, storage_type{std::forward<OnRequest>(on_request)...});
}

template<class Session>
typename router<Session>::mutex_type &router<Session>::get_mutex() const
{
    return m_mutex;
}

template<class Session>
typename router<Session>::method_const_map_pointer router<Session>::get_resource_map() const
{
    return m_method_map;
}

template<class Session>
void router<Session>::add_resource(const std::string &path, const method_type &method, storage_type &&storage)
{
    LOCKABLE_ENTER_TO_WRITE(m_mutex);

    if (!m_method_map) {
        m_method_map = std::make_shared<method_map_type>();
    }
    
    auto &resource_map = m_method_map->insert({method, resource_map_type()}).first->second;
    resource_map.emplace(path, std::move(storage));
}

} // namespace server
