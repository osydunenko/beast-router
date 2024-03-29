#pragma once

ROUTER_NAMESPACE_BEGIN()

template <class Session>
router<Session>::router()
    : m_method_map { new method_map_type {} }
    , m_mutex { new mutex_type {} }
{
    if constexpr (session_type::is_request::value) {
        not_found(&router<Session>::not_found_handler);
    }
}

template <class Session>
ROUTER_DECL typename router<Session>::mutex_type& router<Session>::get_mutex() const
{
    return *m_mutex;
}

template <class Session>
ROUTER_DECL typename router<Session>::mutex_pointer_type
router<Session>::get_mutex_pointer() const
{
    BOOST_ASSERT(m_mutex);
    return m_mutex;
}

template <class Session>
ROUTER_DECL typename router<Session>::method_const_map_pointer
router<Session>::get_resource_map() const
{
    BOOST_ASSERT(m_method_map);
    return m_method_map;
}

template <class Session>
void router<Session>::add_resource(const std::string& path,
    const method_type& method,
    storage_type&& storage)
{
    LOCKABLE_ENTER_TO_WRITE(get_mutex());
    BOOST_ASSERT(m_method_map);

    auto& resource_map = m_method_map->insert({ method, resource_map_type() }).first->second;
    resource_map.insert_or_assign(path, std::move(storage));
}

template <class Session>
void swap(router<Session>& first, router<Session>& second) noexcept
{
    using std::swap;
    swap(first.m_method_map, second.m_method_map);
    swap(first.m_mutex, second.m_mutex);
}

ROUTER_NAMESPACE_END()
