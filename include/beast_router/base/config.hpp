#pragma once

#include <boost/config.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/noncopyable.hpp>
#include <boost/version.hpp>

#if not defined(ROUTER_ASSUME)
#if defined(BOOST_GCC)
#define ROUTER_ASSUME(cond)          \
    do {                             \
        if (!cond)                   \
            __builtin_unreachable(); \
    } while (0)
#else
#define ROUTER_ASSUME(cond) \
    do {                    \
    } while (0)
#endif
#endif

#if defined(BOOST_NO_CXX17_IF_CONSTEXPR) || defined(BOOST_NO_CXX17_FOLD_EXPRESSIONS)
#error The library requires C++17: a conforming compiler is needed
#endif

#define ROUTER_NAMESPACE_BEGIN() namespace beast_router {
#define ROUTER_NAMESPACE_END() }

#if ROUTER_DOXYGEN
#define ROUTER_DECL
#else
#define ROUTER_DECL inline
#endif

#if LINK_SSL
#define SSL
#endif
