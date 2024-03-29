#pragma once

#include <memory>
#include <shared_mutex>
#include <type_traits>

#include "../common/utility.hpp"
#include "config.hpp"

#define LOCKABLE_ENTER_TO_READ(shared_mutex) \
    [[maybe_unused]] const auto& dummy_lock = beast_router::base::lockable::enter_to_read(shared_mutex);

#define LOCKABLE_ENTER_TO_WRITE(shared_mutex) \
    [[maybe_unused]] const auto& dummy_lock = beast_router::base::lockable::enter_to_write(shared_mutex);

ROUTER_BASE_NAMESPACE_BEGIN()

/// Provides a locking functionality to handle a critical section
struct lockable {
    using mutex_type = std::shared_mutex;

    using mutex_pointer_type = std::shared_ptr<mutex_type>;

    using shared_lock_type = std::shared_lock<mutex_type>;

    using unique_lock_type = std::unique_lock<mutex_type>;

    template <class MutexLocker>
    struct ptr_locker : public MutexLocker {
        explicit ptr_locker(mutex_pointer_type holder)
            : MutexLocker { *holder }
            , mutex_holder { holder }
        {
        }

        mutex_pointer_type mutex_holder;
    };

    static shared_lock_type enter_to_read(mutex_type& mutex)
    {
        return shared_lock_type(mutex);
    }

    static unique_lock_type enter_to_write(mutex_type& mutex)
    {
        return unique_lock_type(mutex);
    }

    static ptr_locker<shared_lock_type> enter_to_read(mutex_pointer_type mutex)
    {
        return ptr_locker<shared_lock_type> { mutex };
    }

    static ptr_locker<unique_lock_type> enter_to_write(mutex_pointer_type mutex)
    {
        return ptr_locker<unique_lock_type>(mutex);
    }
};

ROUTER_BASE_NAMESPACE_END()
