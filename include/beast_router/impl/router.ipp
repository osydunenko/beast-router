#pragma once

namespace beast_router {

template<class Session>
router<Session>::router()
    : m_method_map{nullptr}
{
    not_found(&router<Session>::not_found_handler);
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

} // namespace beast_router
