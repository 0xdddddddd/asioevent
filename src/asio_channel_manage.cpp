#include "asio_channel_manage.hpp"
#include "asio_channel.hpp"
#include "asio_channel_basic.hpp"

#include "asio_event.hpp"

namespace ik
{
    asio_channel_manage::asio_channel_manage(asio_event& io_context, asio_channel_basic& channel_function)
        : io_context(io_context)
        , channel_function(channel_function)
        , cnt(0)
    {
    }

    asio_channel_manage::~asio_channel_manage()
    {
    }


    asio::awaitable<void> asio_channel_manage::joins(asio_event& context, asio_channel* channel, const std::error_code& ec)
    {
        try
        {
            io_context.get_parent().post([&]
            {
                sessions.emplace(channel->index(), std::move(channel));
            });
        }
        catch (const std::exception& ex)
        {
            
        }

        co_await channel_function.joins(context, channel, ec);
    }

    asio::awaitable<void> asio_channel_manage::recvice(asio_event& context, asio_channel* channel, const std::error_code& ec, const void* data, std::size_t length)
    {
         co_await channel_function.recvice(context, channel, ec, data, length);
    }

    asio::awaitable<void> asio_channel_manage::leave(asio_event& context, asio_channel* channel, const std::error_code& ec, std::size_t index)
    {
        co_await channel_function.leave(context, channel, ec, index);

        try
        {
            io_context.get_parent().post([&]
            {
                if (sessions.find(index) != sessions.end())
                {
                    sessions.erase(index);
                }
            });
        }
        catch (const std::exception& ex)
        {

        }
    }
}