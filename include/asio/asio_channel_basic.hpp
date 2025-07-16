#ifndef __ASIO_CHANNEL_BASIC_H__
#define __ASIO_CHANNEL_BASIC_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#	pragma once
#endif

#include "ik_platform.h"

namespace ik
{
    class asio_event;
    class asio_channel;

    class asio_channel_basic
    {
    public:
        virtual asio::awaitable<void> joins(asio_event& context, asio_channel* channel, const std::error_code& ec) { co_await asio::this_coro::executor; };
        virtual asio::awaitable<void> recvice(asio_event& context, asio_channel* channel, const std::error_code& ec, const void* data, std::size_t length) { co_await asio::this_coro::executor; };
        virtual asio::awaitable<void> leave(asio_event& context, asio_channel* channel, const std::error_code& ec, std::size_t index) { co_await asio::this_coro::executor; };
    };
}

#endif // __ASIO_CHANNEL_BASIC_H__