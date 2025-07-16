#ifndef __ASIO_TCP_SERVER_H__
#define __ASIO_TCP_SERVER_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#	pragma once
#endif

#include "asio_context.hpp"
#include "asio_context_thread_pool.hpp"

#include "asio_observer.hpp"
#include "asio_utils.hpp"

#include "asio_session.hpp"
#include "asio_tcp_server_basic.hpp"

#include <asio.hpp>
#include <functional>
#include <utility>
#include <atomic>
#include <memory>

namespace ik
{
    class asio_tcp_server
    {
    public:
        explicit asio_tcp_server(asio_context& io_context, asio_binder& binder)
            : io_context(io_context)
            , binder(binder)
            , server_basic(std::make_shared<asio_tcp_server_basic>(io_context, binder))
            , index(0)
        {
            using namespace std::placeholders;
            binder | std::make_pair(bind_type::init, std::bind(&asio_tcp_server::init, this, _1));
            binder | std::make_pair(bind_type::stop, std::bind(&asio_tcp_server::stop, this, _1));
            binder | std::make_pair(bind_type::accept, std::bind(&asio_tcp_server::join, this, _1, _2, _3));
            binder | std::make_pair(bind_type::recv, std::bind(&asio_tcp_server::receive, this, _1, _2, _3, _4));
            binder | std::make_pair(bind_type::disconnect, std::bind(&asio_tcp_server::leave, this, _1, _2, _3));
        }
        virtual ~asio_tcp_server() = default;
    public:
        void start()
        {
            server_basic->init(1, 0);
            server_basic->async_listen(6666);
        }
    private:
        void init(asio_acceptor& acceptor)
        {
            printf("server init\n");
        }

        void stop(asio_acceptor& acceptor)
        {
            printf("server stop\n");
        }

        void join(asio_context& context, asio_session& session, asio_error& ec)
        {
            assert(io_context.running_in_this_thread());
            return asio::co_spawn(io_context, join_client(session, session.index()), asio::detached);
        }

        void receive(asio_context& context, asio_session& session, const char* buf, std::size_t n)
        {
            session.async_writer("123456\n");
        }

        void leave(asio_context& context, asio_session& session, asio_error& ec)
        {
            assert(io_context.running_in_this_thread());
            return asio::co_spawn(io_context, leave_client(session.index()), asio::detached);
        }

    private:
        asio::awaitable<void> join_client(asio_session& session, std::size_t id)
        {
            co_await asio::this_coro::executor;
            sessions.emplace(id, std::make_shared<asio_session>(std::move(session))).first->second->init();
        }

        asio::awaitable<void> leave_client(std::size_t id)
        {
            co_await asio::this_coro::executor, sessions.erase(id);
        }
    private:
        asio_context&                                                                          io_context;
        asio_binder&                                                                           binder;
        std::shared_ptr<asio_tcp_server_basic>                                                 server_basic;
        std::atomic_size_t                                                                     index;
        std::unordered_map<std::size_t, std::shared_ptr<asio_session>>                         sessions;
    };
}

#endif // __ASIO_TCP_SERVER_H__