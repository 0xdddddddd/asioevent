#ifndef __ASIO_SLEEP_H__
#define __ASIO_SLEEP_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#	pragma once
#endif

#include "asio_context.hpp"
#include "asio_timer.hpp"

namespace ik
{
    class asio_sleep : public asio_steady_timer
    {
    public:
        explicit asio_sleep(const asio::any_io_executor& io_executor)
            : asio_steady_timer(io_executor)
        {

        }

        explicit asio_sleep(asio_context& io_context)
            : asio_steady_timer(io_context)
        {
        }

        explicit asio_sleep(asio_context& io_context, const clock_handler& coro)
            : asio_steady_timer(io_context, coro)
        {
        }
    };
}

#endif