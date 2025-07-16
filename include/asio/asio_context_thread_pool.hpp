#ifndef __ASIO_CONTEXT_THREAD_POOL_H__
#define __ASIO_CONTEXT_THREAD_POOL_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#	pragma once
#endif

#include "asio_context.hpp"
#include "asio_context_thread.hpp"

#include <asio.hpp>
#include <vector>
#include <memory>
#include <ranges>

namespace ik
{
    class asio_context_thread_pool
    {
    public:
        explicit asio_context_thread_pool(asio_context& io_context)
            : io_context(io_context)
        {

        }

        virtual ~asio_context_thread_pool()
        {

        }
    public:
        /**
         * @brief Initialize the root and child threads for the IO context.
         * @param thr_root - The number of root threads to create.
         * @param thr_child - The number of child threads per root thread.
         * @note This function creates `thr_root` root threads, each managing `thr_child` child threads.
         *       Each root thread is associated with an `asio_event_thread` instance.
         */
        void init(std::size_t ctx_cnt, std::size_t thrd_cnt = 0)
        {
            for (std::size_t i = 0; i < ctx_cnt; ++i)
            {
                io_context_thread.emplace(io_context_thread.begin() + i, std::make_shared<asio_context_thread>(io_context, thrd_cnt, i, 0, 1024));
            }

            for (const auto& context : io_context_thread)
            {
                if (context->init())
                {
                    continue;
                }
            }
        }

        /**
         * @brief Stop all root threads and their associated child threads.
         * @note This function stops each root thread by calling its `stop` method.
         */
        void stop()
        {
            for (const auto& context : io_context_thread)
            {
                if (context->stop())
                {
                    continue;
                }
            }
        }

        /**
         * @brief Get the appropriate IO context based on the default context index.
         * @return Returns a reference to the selected `asio_event` context.
         * @note This function calls `get_context(std::size_t)` with the index obtained from `get_context_idx()`.
         */
        asio_context& get_context()
        {
            return get_context(get_context_idx());
        }

        /**
         * @brief Get the appropriate IO context based on the provided index.
         * @param n - The index used to select the context.
         * @return Returns a reference to the selected `asio_event` context.
         * @note If the index has the highest bit set, it returns the global `io_context`.
         *       Otherwise, it returns the context from the corresponding root thread.
         */
        asio_context& get_context(std::size_t n)
        {
            return ((n & (static_cast<std::size_t>(1) << (sizeof(std::uint32_t) * 8 - 1))) != 0) ? io_context : io_context_thread[n % io_context_thread.size()]->get_context();
        }

        /**
         * @brief Get the index of the most suitable IO context.
         * @return Returns the index of the context with the least number of active tasks.
         * @note This function filters root threads that have not reached their maximum task capacity
         *       and selects the one with the least number of active tasks.
         *       If no valid context is found, it returns the maximum value of `std::int32_t`.
         */
        std::size_t get_context_idx()
        {
            // Filter root threads that have not reached their maximum task capacity.
            auto valid_view = io_context_thread | std::views::filter([] (const std::shared_ptr<asio_context_thread>& task) {
                return task->task_num.load() < task->task_max.load();
            });

            // If no valid context is found, return the maximum value of `std::int32_t`.
            if (valid_view.empty())
            {
                return std::numeric_limits<std::int32_t>::max() + 1;
            }

            // Create a vector of valid root threads.
            std::vector<std::reference_wrapper<std::shared_ptr<asio_context_thread>>> valid_tasks
            {
                valid_view.begin(), valid_view.end()
            };

            // Select the root thread with the least number of active tasks.
            return std::ranges::min(valid_tasks,
                                    {},
                                    [] (const std::shared_ptr<asio_context_thread>& task)
            {
                return task->task_num.load();
            }).get()->get_idx();
        }
    private:
        asio_context&                                                       io_context;
        std::vector<std::shared_ptr<asio_context_thread>>                     io_context_thread;
    };
}

#endif // __ASIO_CONTEXT_THREAD_POOL_H__