#include "asio_tcp_server.hpp"

#include "asio_event.hpp"
#include "asio_event_thread_pool.hpp"

#include "asio_channel.hpp"
#include "asio_channel_manage.hpp"

namespace ik
{
    asio_tcp_server::asio_tcp_server(asio_event& io_context, const std::shared_ptr<asio_channel_manage>& channel_manage, const asio::ip::tcp& protocol, u16 port)
        : io_context(io_context)
        , cnt(0)
        , acceptor(io_context, { protocol, port })
        , thread_pool(std::make_unique<asio_event_thread_pool>(io_context))
        , channel_manage(channel_manage)
    {
        thread_pool->init(2);
    }

    asio_tcp_server::~asio_tcp_server()
    {

    }

    void asio_tcp_server::start()
    {
        asio::co_spawn(io_context, start(acceptor), asio::detached);
    }

    asio::awaitable<void> asio_tcp_server::start(asio::ip::tcp::acceptor& acceptor)
    {
        asio::error_code ec;

        try
        {
            for (; acceptor.is_open();)
            {
                asio::ip::tcp::socket socket = co_await acceptor.async_accept(asio::redirect_error(asio::use_awaitable, ec));

                if (ec)
                {
                    socket.close();
                    continue;
                }
                else
                {
                    co_await accept(thread_pool->event(), std::move(socket));
                }
            }
        }
        catch (const std::exception& ec)
        {
            printf("%s\n", ec.what());
        }
    }

    void asio_tcp_server::stop()
    {
        if (io_context.running_in_this_thread())
        {
            acceptor.close();
        }
        else
        {
            io_context.post(std::bind(&asio_tcp_server::stop, this));
        }
    }

    asio::awaitable<void> asio_tcp_server::accept(asio_event& context, asio::ip::tcp::socket&& socket)
    {
        if (channel_manage)
        {
            co_await channel_manage->joins(context, std::ref(*new asio_channel(context, cnt.fetch_add(1), channel_manage, std::move(socket))));
        }
        else
        {
            socket.close();
        }
    }
}