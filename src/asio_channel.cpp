#include "asio_channel.hpp"
#include "asio_channel_manage.hpp"



namespace ik
{
    asio_channel::asio_channel(asio_event& io_context, asio_binder& binder, asio::ip::tcp::socket&& socket, size_t index)
        : self(std::ref(*this))
        , io_context(io_context)
        , io_strand(io_context.get_executor())
        , binder(binder)
        , socket(std::move(socket))
        , index_(index)
        , io_sleep(std::make_shared<asio_sleep>(io_context))
    {
        asio::co_spawn(io_context, reader(), asio::bind_executor(io_strand, asio::detached));
        asio::co_spawn(io_context, writer(), asio::bind_executor(io_strand, asio::detached));
    }

    asio_channel::~asio_channel()
    {
        printf("\n");
    }

    void asio_channel::async_send(const std::string& buffer)
    {
        if (io_context.running_in_this_thread())
        {
            asio::co_spawn(io_context,
                           async_send_coro(buffer),
                           asio::bind_executor(io_strand, asio::detached));
        }
        else
        {
            io_context.post(std::bind(&asio_channel::async_send, this, buffer));
        }
    }

    void asio_channel::async_writer(const std::string& buffer)
    {
        if (io_context.running_in_this_thread())
        {
            if (socket.is_open())
            {
                io_msdeque.push_back(buffer);
                io_sleep->cancel_one();
            }
        }
        else
        {
            io_context.post(std::bind(&asio_channel::async_writer, this, buffer));
        }
    }

    void asio_channel::close()
    {
        asio::error_code ec;

        try
        {
            if (io_context.running_in_this_thread())
            {
                if (socket.is_open() == false)
                {
                    return;
                }

                if (socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec))
                {
                    return;
                }

                if (socket.close(ec))
                {
                    return;
                }

                if (io_sleep->cancel_one(ec))
                {
                    return;
                }
            }
            else
            {
                io_context.post(std::bind(&asio_channel::close, this));
            }
        }
        catch (const std::exception&)
        {

        }
    }

    asio::awaitable<void> asio_channel::async_send_coro(const std::string& buffer)
    {
        asio::error_code ec;

        try
        {
            if (socket.is_open())
            {
                if (co_await socket.async_send(asio::buffer(buffer.c_str(), buffer.size()),
                    asio::bind_executor(io_strand,
                    asio::redirect_error(asio::use_awaitable, ec))) < 0 || ec)
                {
                    this->close();
                }
            }
        }
        catch (const std::exception&)
        {

        }
    }

    asio::awaitable<void> asio_channel::reader()
    {
        asio::error_code ec;
        char data[8 * 1024];

        try
        {
            for (size_t n = 0; socket.is_open();)
            {
                if ((n = co_await socket.async_read_some(asio::buffer(data, sizeof(data)),
                    asio::bind_executor(io_strand,
                    asio::redirect_error(asio::use_awaitable, ec)))) == 0)
                {
                    if (ec)
                    {
                        this->close();
                        break;
                    }
                }

                binder.notify(bind_type::recv, io_context, this, ec, data, n);
            }

            binder.notify(bind_type::disconnect, io_context, this, ec, index_);
        }
        catch (std::exception&)
        {

        }
    }

    asio::awaitable<void> asio_channel::writer()
    {
        asio::error_code ec;

        try
        {
            for (; socket.is_open();)
            {
                if (io_msdeque.empty())
                {
                    co_await io_sleep->async_wait(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::duration::max()));
                }
                else
                {
                    co_await socket.async_send(asio::buffer(io_msdeque.front()),
                                               asio::bind_executor(io_strand, asio::redirect_error(asio::use_awaitable, ec)));
                    io_msdeque.pop_front();
                }
            }
        }
        catch (std::exception&)
        {

        }
    }
}