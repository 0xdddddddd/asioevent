#include "asio_tcp_server.hpp"

#include "asio_event.hpp"
#include "asio_event_thread_pool.hpp"

namespace ik
{
    asio_tcp_server::asio_tcp_server(asio_event& io_context, const asio::ip::tcp& protocol, u16 port)
        : io_context(io_context)
        , cnt(0)
        , acceptor(io_context, { protocol, port })
        , io_strand(io_context.get_executor())
        , binder(std::make_shared<asio_binder>())
    {

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
            for (; acceptor.is_open(); )
            {
                if (asio::ip::tcp::socket socket(co_await asio::this_coro::executor); co_await acceptor.async_accept(socket, asio::bind_executor(io_strand, asio::redirect_error(asio::use_awaitable, ec))), ec)
                {
                    switch (ec.value())
                    {
                        case asio::error::not_socket: break;
                        case asio::error::bad_descriptor: break;
                        default: socket.close(); break;
                    }
                    continue;
                }
                else
                {
                    binder->notify(bind_type::accept, io_context);
                    //co_await channel_manage.joins(io_context, new asio_channel(io_context, *binder, std::move(socket), cnt.fetch_add(1)), ec);
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
}