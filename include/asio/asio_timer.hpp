#ifndef __ASIO_TIMER_H__
#define __ASIO_TIMER_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#	pragma once
#endif

#include "asio_context.hpp"
#include "asio_traits.hpp"

#include <atomic>
#include <memory>
#include <bit>

namespace ik
{
    template<typename T = asio::steady_timer>
    class asio_timer_basic : public std::enable_shared_from_this<asio_timer_basic<T>>, T
    {
    public:
        using timer_type = T;
    public:
        //using clock_handler = std::function<asio::awaitable<void>(const std::shared_ptr<asio_timer_basic<timer_type>>&)>;
        using clock_handler = std::function<asio::awaitable<void>(asio_timer_basic<timer_type>&)>;
        using clock_type = asio_timer_traits<timer_type>::type;
    public:
        explicit asio_timer_basic(const asio::any_io_executor& io_executor)
            : asio_timer_basic(static_cast<asio_context&>(asio::query(io_executor, asio::execution::context)), clock_handler())
        {

        }

        explicit asio_timer_basic(asio_context& io_context)
            : asio_timer_basic(io_context, clock_handler())
        {

        }

        explicit asio_timer_basic(asio_context& io_context, const clock_handler& coro)
            : timer_type(io_context)
            , self(*this)
            , io_context(io_context)
            , state(true)
            , handler(coro)
        {
        }
        virtual ~asio_timer_basic()
        {
            
        };
    public:
        template <typename Token = asio::default_completion_token_t<timer_type::executor_type>>
        asio::awaitable<void> async_wait(const clock_type& expiry_time, Token&& token = asio::default_completion_token_t<timer_type::executor_type>())
        {
            timer_type::expires_after(expiry_time), co_await timer_type::async_wait(std::forward<Token>(token));
        }

        template <typename Token = asio::default_completion_token_t<timer_type::executor_type>>
        asio::awaitable<void> async_handler_wait(size_t expiry_time, Token&& token = asio::default_completion_token_t<timer_type::executor_type>())
        {
            co_await async_handler_wait(std::chrono::milliseconds(expiry_time), std::forward<Token>(token));
        }

        /**
        * @brief Coroutine to asynchronously wait for the timer to expire.
        * @param expiry_time - The expiry time as a `clock_type` duration.
        * @return Returns an `asio::awaitable<void>` that completes when the timer is stopped or canceled.
        * @note This coroutine continuously waits for the timer to expire and invokes a callback (`this_coro`) if provided.
        */
        template <typename Token = asio::default_completion_token_t<timer_type::executor_type>>
        asio::awaitable<void> async_handler_wait(const clock_type& expiry_time, Token&& token = asio::default_completion_token_t<timer_type::executor_type>())
        {
            for (timer_type::expires_after(expiry_time), state.store(true);
                 state.load(); co_await timer_type::async_wait(std::forward<Token>(token)))
            {
                if (handler != nullptr)
                {
                    co_await handler(std::forward<asio_timer_basic<timer_type>>(self));
                }
            }
        }

        void restart()
        {
            state.exchange(true);
        }

        bool is_open()
        {
            return state.load();
        }

        bool stop()
        {
            try
            {
                return state.store(false), timer_type::cancel() >= 0;
            }
            catch (const std::exception&)
            {
                // Exception handling (e.g., logging) can be added here.
            }

            return false;
        }

        /**
         * @brief Cancel the timer and stop any pending asynchronous operations.
         * @note This function sets the internal state to false and cancels the timer.
         */
        bool cancel()
        {
            try
            {
                return timer_type::cancel() >= 0;
            }
            catch (const std::exception&)
            {
                // Exception handling (e.g., logging) can be added here.
            }

            return false;
        }

        /**
         * @brief Cancel one pending asynchronous operation and stop the timer.
         * @return Returns the number of operations canceled (0 or 1).
         * @note This function sets the internal state to false and cancels one pending operation.
         */
        bool cancel_one()
        {
            try
            {
                return timer_type::cancel_one() >= 0;
            }
            catch (const std::exception&)
            {
                // Exception handling (e.g., logging) can be added here.
            }

            return false;
        }

        /**
         * @brief Cancel one pending asynchronous operation and stop the timer, with error handling.
         * @param ec - An `asio::error_code` to store any errors that occur during cancellation.
         * @return Returns the number of operations canceled (0 or 1).
         * @note This function sets the internal state to false and cancels one pending operation,
         *       storing any errors in the provided `asio::error_code`.
         */
        bool cancel_one(asio::error_code& ec)
        {
            try
            {
                return timer_type::cancel_one(ec) >= 0;
            }
            catch (const std::exception&)
            {
                // Exception handling (e.g., logging) can be added here.
            }

            return false;
        }
    public:
        asio_timer_basic<timer_type>&                   self;
        asio_context&                                   io_context;
        std::atomic_bool                                state;
        clock_handler                                   handler;
    };

    using asio_steady_timer = asio_timer_basic<asio::steady_timer>;
    using asio_system_timer = asio_timer_basic<asio::system_timer>;
}

#endif // __ASIO_TIMER_H__