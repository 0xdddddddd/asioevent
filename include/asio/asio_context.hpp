#ifndef __ASIO_CONTEXT_H__
#define __ASIO_CONTEXT_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#	pragma once
#endif

#include <asio.hpp>

#include <atomic>
#include <vector>

namespace ik
{
    class asio_context : public asio::io_context
    {
    public:
        explicit asio_context(asio_context* parent = nullptr, size_t id = 0)
            : parent(parent == nullptr ? std::ref(*this) : std::ref(*parent))
            , guard(asio::make_work_guard(*this))
            , id(id)
        {

        }
        virtual ~asio_context() { stop(); }
    public:
        void stop() { guard.reset(); };
        /**
         * @brief Check if the current thread is running the event loop of the `io_context`.
         * @return Returns `true` if the current thread is running the event loop, otherwise `false`.
         */
        bool running_in_this_thread() noexcept
        {
            return asio::io_context::get_executor().running_in_this_thread();
        }

        /**
         * @brief Dispatch the event loop to run on multiple threads.
         * @param thrd_size - The number of threads to spawn for running the event loop.
         * @note If `thrd_size` is greater than 0, the event loop will run on the specified number of threads.
         *       Otherwise, the event loop will run on the current thread.
         */
        void run(std::size_t task_cnt = 0) noexcept
        {
            try
            {
                if (task_cnt)
                {
                    // Spawn the specified number of threads to run the event loop.
                    for (std::size_t i = 0; i < task_cnt; ++i)
                    {
                        thread.emplace_back([=] () mutable {
                            asio::io_context::run();
                        }).detach();
                    }
                }

                // Run the event loop on the current thread.
                asio::io_context::run();
            }
            catch (const std::exception&)
            {

            }
        }

        /**
         * @brief Get the index of the current instance.
         * @return Returns the index as a `size_t` value.
         */
        size_t index() const noexcept
        {
            return id.load();
        };

        /**
         * @brief Get a reference to the parent `asio_context` object.
         * @return Returns a reference to the parent `asio_context` object.
         */
        asio_context& get_parent() noexcept
        {
            return std::ref(parent);
        };
    private:
        asio_context&                                              parent;
        asio::executor_work_guard<asio::io_context::executor_type> guard;
        std::atomic_size_t                                         id;
        std::vector<std::jthread>                                  thread;
    };
}

#endif // __ASIO_CONTEXT_H__