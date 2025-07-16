#ifndef __ASIO_SESSION_H__
#define __ASIO_SESSION_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#	pragma once
#endif

#include "ik_platform.h"
#include "asio_channel_basic.hpp"

namespace ik
{
    class asio_event;
    class asio_channel;
    class asio_channel_basic;

    class asio_channel_manage : public asio_channel_basic, std::enable_shared_from_this<asio_channel_manage>
    {
    public:
        explicit asio_channel_manage(asio_event& io_context, asio_channel_basic& channel_function);
        virtual ~asio_channel_manage();
    public:
        virtual asio::awaitable<void> joins(asio_event& context, asio_channel* channel, const std::error_code& ec) override;
        virtual asio::awaitable<void> recvice(asio_event& context, asio_channel* channel, const std::error_code& ec, const void* data, std::size_t length) override;
        virtual asio::awaitable<void> leave(asio_event& context, asio_channel* channel, const std::error_code& ec, std::size_t index) override;
    private:
        asio_event& io_context;
        std::mutex mutex;
        asio_channel_basic& channel_function;
        std::atomic_size_t cnt;
        std::unordered_map<size_t, std::shared_ptr<asio_channel>> sessions;
    };
}
#endif // __ASIO_SESSION_H__