#ifndef __ASIO_CONTEXT_THREAD_H__
#define __ASIO_CONTEXT_THREAD_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#	pragma once
#endif

#include "asio_context.hpp"

#include <asio.hpp>
#include <atomic>
#include <semaphore>
#include <memory>

namespace ik
{
    class asio_context_thread
    {
    public:
        explicit asio_context_thread(asio_context& io_context, std::size_t task_cnt, size_t task_idx, std::size_t task_num, std::size_t task_max)
            : io_context(io_context)
            , task_cnt(task_cnt)
            , task_idx(task_idx)
            , task_num(task_num)
            , task_max(task_max)
            , task_tick(0)
            , semaphore(0)
        {
        }
        virtual ~asio_context_thread() = default;
    private:
        asio_context_thread& operator=(const asio_context_thread&) = delete;
    public:
        /**
         * @brief Initialize the worker thread and semaphore.
         * @return Returns `true` if initialization is successful within 3 seconds, otherwise `false`.
         * @note This function creates a worker thread if it doesn't already exist and attempts to acquire the semaphore within a timeout.
         */
        bool init()
        {
            if (worker == nullptr)
            {
                worker = std::make_unique<std::jthread>(std::bind(&asio_context_thread::dispatch, this, std::placeholders::_1));
            }

            return semaphore.try_acquire_for(std::chrono::seconds(3));
        }

        /**
         * @brief Stop the worker thread.
         * @return Returns `true` if the worker thread is successfully requested to stop, otherwise `false`.
         * @note This function checks if the worker thread is joinable and requests it to stop.
         */
        bool stop()
        {
            if (worker->joinable())
            {
                if (worker->request_stop())
                {
                    return true;
                }
            }

            return false;
        }

        /**
         * @brief Get the current task index and increment task counters.
         * @return Returns the current task index as a `std::size_t` value.
         * @note This function increments both `task_num` and `task_tick` counters and returns the current `task_idx`.
         */
        std::size_t get_idx()
        {
            return task_num.fetch_add(1, std::memory_order_relaxed), task_tick.fetch_add(1, std::memory_order_relaxed), task_idx.load();
        }

        /**
         * @brief Get the associated `asio_event` context.
         * @return Returns a reference to the `asio_event` context.
         * @note If `io_thread_context` is initialized, it returns a reference to it; otherwise, it returns a reference to `io_context`.
         */
        asio_context& get_context()
        {
            return io_thread_context ? std::ref(*io_thread_context) : std::ref(io_context);
        }

        /**
         * @brief Dispatch the event loop on the worker thread.
         * @param stop_token - A stop token to check for stop requests.
         * @note This function initializes the `io_thread_context` if it doesn't exist and runs the event loop until a stop is requested.
         */
        void dispatch(const std::stop_token& stop_token)
        {
            if (io_thread_context == nullptr)
            {
                io_thread_context = std::make_unique<asio_context>(std::addressof(io_context), task_idx.load());
            }

            if (io_thread_context == nullptr)
            {
                return;
            }

            while (!stop_token.stop_requested())
            {
                semaphore.release(), io_thread_context->run(task_cnt.load());
            }
        }
    public:
        asio_context&                                                       io_context;
        std::atomic_size_t                                                  task_cnt;                    // 线程数量
        std::atomic_size_t                                                  task_idx;                    // 所属任务
        std::atomic_size_t                                                  task_num;                    // 当前任务
        std::atomic_size_t                                                  task_max;                    // 最大任务
        std::atomic_ullong                                                  task_tick;                   // 累计使用
        std::binary_semaphore                                               semaphore;
        std::unique_ptr<std::jthread>                                       worker;
        std::unique_ptr<asio_context>                                       io_thread_context;
    };
}

#endif // __ASIO_CONTEXT_THREAD_H__