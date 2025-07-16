#include "asio_tcp_client.hpp"

#include "asio_event.hpp"

#include "asio_channel.hpp"
#include "asio_channel_basic.hpp"

namespace ik
{
    asio_tcp_client::asio_tcp_client(asio_event& io_context, std::size_t ider)
        : io_context(io_context)
        , ider(ider)
        , io_strand(io_context.get_executor())
        , channel()
        , binder(std::make_shared<asio_binder>())
    {

    }

    asio_tcp_client::~asio_tcp_client()
    {

    }

    void asio_tcp_client::async_connect(const std::string& address, u16 port)
    {
        if (channel)
        {
            return;
        }

        asio::error_code ec;

        try
        {
            asio::co_spawn(io_context, [=] () -> asio::awaitable<void> {
                co_await async_connect_coro(asio::ip::tcp::endpoint(asio::ip::make_address(address), port));
            }, asio::bind_executor(io_strand, asio::redirect_error(asio::detached, ec)));

        }
        catch (const std::exception& ex)
        {

        }
    }

    void asio_tcp_client::async_connect_with_resolver(const std::string& address, const std::string& scheme)
    {
        if (channel)
        {
            return;
        }

        asio::error_code ec;

        try
        {
            asio::co_spawn(io_context, [=] () -> asio::awaitable<void> {
                (void)co_await async_connect_with_resolver_coro(asio::ip::tcp::resolver::query(address, scheme));
            }, asio::bind_executor(io_strand, asio::redirect_error(asio::detached, ec)));
        }
        catch (const std::exception& ex)
        {

        }
    }

    void asio_tcp_client::async_send(const std::string& buffer)
    {
        if (channel)
        {
            channel->async_send(buffer);
        }
    }

    void asio_tcp_client::async_writer(const std::string& buffer)
    {
        if (channel)
        {
            channel->async_writer(buffer);
        }
    }

    void asio_tcp_client::close()
    {
        if (channel)
        {
            channel->close();
        }
    }

    asio::awaitable<bool> asio_tcp_client::async_connect_coro(const asio::ip::tcp::endpoint& endpoint)
    {
        asio::error_code ec;
        asio::ip::tcp::socket socket(co_await asio::this_coro::executor);

        try
        {
            if (co_await socket.async_connect(endpoint,
                asio::bind_executor(io_strand,
                asio::redirect_error(asio::use_awaitable, ec))), ec)
            {
                switch (ec.value())
                {
                    case asio::error::connection_refused: break;
                    default: break;
                }

                socket.close();
            }

            co_await binder->notify_coro(bind_type::connect, io_context, ec.value());
        }
        catch (const std::exception& ex)
        {

        }

        co_return channel ? true : false;
    }

    asio::awaitable<void>  asio_tcp_client::async_connect_with_resolver_coro(const asio::ip::tcp::resolver::query& query)
    {
        asio::error_code ec;
        asio::ip::tcp::resolver resolver(co_await asio::this_coro::executor);

        try
        {
            for (const auto& entry : co_await resolver.async_resolve(query,
                 asio::bind_executor(io_strand, asio::redirect_error(asio::use_awaitable, ec))))
            {
                if (co_await async_connect_coro(entry.endpoint()))
                {
                    break;
                }
            }

            if (ec)
            {
                printf("%d\n", ec.value());
            }
        }
        catch (const std::exception& ex)
        {

        }
    }
}