#include "asio_event_thread.hpp"
#include "asio_event.hpp"

namespace ik
{
    bool asio_event_thread::init()
    {
        if (worker == nullptr)
        {
            worker = std::make_unique<std::jthread>(std::bind(&asio_event_thread::dispatch, this, std::placeholders::_1));
        }

        return semaphore.try_acquire_for(std::chrono::seconds(3));
    }

    bool asio_event_thread::stop()
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

    void asio_event_thread::dispatch(const std::stop_token& stop_token)
    {
        if (io_thread_context == nullptr)
        {
            io_thread_context = std::make_unique<asio_event>(std::addressof(io_context), task_idx.load());
        }

        if (io_thread_context == nullptr)
        {
            return;
        }

        while (!stop_token.stop_requested())
        {
            semaphore.release(), io_thread_context->dispatch(task_size.load());
        }
    }
}