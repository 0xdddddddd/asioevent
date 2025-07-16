#include "asio_event_thread.hpp"
#include "asio_event.hpp"

namespace ik
{
    asio_event_thread::asio_event_thread(size_t id, asio_event& base)
        : id(id)
        , base(base)
        , loop(nullptr)
        , cnt(0)
    {

    }

    asio_event_thread::~asio_event_thread()
    {

    }

    bool asio_event_thread::init()
    {
        if (worker == nullptr)
        {
            worker = std::make_unique<std::jthread>(std::bind(&asio_event_thread::dispatch, this, std::placeholders::_1));
        }

        return cnt.try_acquire_for(std::chrono::seconds(3));
    }

    bool asio_event_thread::stop()
    {
        if (worker && worker->request_stop())
        {
            if (loop)
            {
                loop->post([&] ()
                {
                    loop->stop();
                });
                return true;
            }
        }

        return false;
    }

    asio_event& asio_event_thread::event()
    {
        return (worker && worker->joinable()) ? std::ref(*loop) : std::ref(base);
    }

    void asio_event_thread::dispatch(const std::stop_token& stop_token)
    {
        if (loop == nullptr)
        {
            loop = std::make_unique<asio_event>(id.load());
        }

        printf("start thread id: %x\n", GetCurrentThreadId());

        if (loop && loop->init())
        {
            while (!stop_token.stop_requested())
            {
                cnt.release(), loop->dispatch();
            }
        }

        printf("end thread id: %x\n", GetCurrentThreadId());
    }
}