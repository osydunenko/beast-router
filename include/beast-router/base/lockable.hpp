#pragma once

#include <shared_mutex>

#define LOCKABLE_ENTER_TO_READ(shared_mutex) \
    const auto &dummy_lock = beast_router::base::lockable::enter_to_read(shared_mutex); \
    static_cast<void>(dummy_lock);

#define LOCKABLE_ENTER_TO_WRITE(shared_mutex) \
    const auto &dummy_lock = beast_router::base::lockable::enter_to_write(shared_mutex); \
    static_cast<void>(dummy_lock);

namespace beast_router {
namespace base {

struct lockable
{
    using mutex_type = std::shared_mutex;

    using shared_lock_type = std::shared_lock<mutex_type>;

    using unique_lock_type = std::unique_lock<mutex_type>;

    static shared_lock_type enter_to_read(mutex_type &mutex)
    {
        return shared_lock_type(mutex);
    }

    static unique_lock_type enter_to_write(mutex_type &mutex)
    {
        return unique_lock_type(mutex);
    }
};

} // namespace base
} // namespace beast_router
