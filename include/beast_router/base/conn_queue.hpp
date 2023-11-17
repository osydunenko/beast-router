#pragma once

#include <memory>
#include <queue>
#include <utility>

namespace beast_router {
namespace base {

    /// Handles packages dispatching to be sent
    template <class Impl>
    class conn_queue {
        struct wrk {
            virtual ~wrk() = default;
            virtual void operator()() = 0;
        };

    public:
        using self_type = conn_queue<Impl>;

        using impl_type = Impl;

        explicit conn_queue(impl_type& impl)
            : m_impl { impl }
            , m_items {}
        {
        }

        template <class Response>
        void operator()(Response&& rsp)
        {
            struct wrk_impl : wrk {
                using response_type = std::decay_t<Response>;

                wrk_impl(impl_type& impl, Response&& rsp)
                    : m_impl { impl }
                    , m_response { std::forward<Response>(rsp) }
                {
                }

                void operator()() override { m_impl.do_write(m_response); }

                impl_type& m_impl;
                response_type m_response;
            };

            m_items.push(
                std::make_unique<wrk_impl>(m_impl, std::forward<Response>(rsp)));

            if (m_items.size() == 1) {
                do_handle();
            }
        }

        void on_write()
        {
            m_items.pop();
            if (m_items.size()) {
                do_handle();
            }
        }

    protected:
        void do_handle()
        {
            BOOST_ASSERT(m_items.size());
            (*m_items.front())();
        }

    private:
        using wrk_ptr_type = std::unique_ptr<wrk>;

        impl_type& m_impl;
        std::queue<wrk_ptr_type> m_items;
    };

} // namespace base
} // namespace beast_router
