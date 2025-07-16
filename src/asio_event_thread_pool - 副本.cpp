#include "asio_event_thread_pool.hpp"
#include "asio_event_thread.hpp"
#include "asio_event.hpp"

namespace ik
{
    asio_event_thread_pool::asio_event_thread_pool(asio_event& base)
        : base(base)
        , cnt(0)
    {

    }

    asio_event_thread_pool::~asio_event_thread_pool()
    {

    }

    void asio_event_thread_pool::init(size_t thr_size)
    {
        for (size_t i = 0; i < thr_size; ++i)
        {
            events.emplace(i, std::make_unique<asio_event_thread>(i, base));
        }

        for (const auto& [index, event] : events)
        {
            if (event->init())
            {
                continue;
            }
        }
    }

    bool asio_event_thread_pool::stop(size_t n)
    {
        return events[n]->stop();
    }

    asio_event& asio_event_thread_pool::event()
    {
        return event(cnt.fetch_add(1));
    }

    asio_event& asio_event_thread_pool::event(size_t n)
    {
        return events[n % events.size()]->event();
    }
}