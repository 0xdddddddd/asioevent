#include "asio_event.hpp"
#include "asio_timer.hpp"

namespace ik
{
    asio_event::asio_event(asio_event* parent, size_t io_idx)
        : io_parent(parent == nullptr ? std::ref(*this) : std::ref(*parent))
        , io_guard(asio::make_work_guard(*this))
        , io_idx(io_idx)
        , state(true)
    {
        printf("IOÏß³Ì£º%X\n", GetCurrentThreadId());
    }

    asio_event::~asio_event()
    {
    }

    void asio_event::stop()
    {
        if (state.load())
        {
            state.store(false), asio::io_context::stop();
        }
    }

    bool asio_event::running_in_this_thread()
    {
        return asio::io_context::get_executor().running_in_this_thread();
    }

    void asio_event::dispatch(std::size_t thr_size)
    {
        if (thr_size)
        {
            for (std::size_t i = 0; i < thr_size; ++i)
            {
                io_thread.emplace_back([&] {
                    asio::io_context::run();
                }).detach();
            }
        }

        asio::io_context::run();
    }
}