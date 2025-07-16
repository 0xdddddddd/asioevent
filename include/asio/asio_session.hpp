#ifndef __ASIO_SESSION_H__
#define __ASIO_SESSION_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#	pragma once
#endif

#include "asio_context.hpp"
#include "asio_sleep.hpp"
#include "asio_observer.hpp"
#include "asio_utils.hpp"

#include <asio.hpp>

namespace ik
{
    class asio_session : public std::enable_shared_from_this<asio_session>
    {
    public:
        explicit asio_session(asio_context& io_context, asio_binder& binder, asio_socket& stream_socket, std::size_t id)
            : self(std::ref(*this))
            , io_context(io_context)
            , io_strand(io_context.get_executor())
            , binder(binder)
            , stream_socket(std::move(stream_socket))
            , id(id)
            , sleep(std::make_shared<asio_sleep>(io_context))
        {
            // transfer the initialization action to avoid not being able to use shared_from_this() directly in the constructor
        }
        explicit asio_session(asio_session&& other) noexcept
            : self(std::ref(*this))
            , io_context(other.io_context)
            , io_strand(std::move(other.io_strand))
            , binder(other.binder)
            , stream_socket(std::move(other.stream_socket))
            , id(other.id)
            , sleep(std::move(other.sleep))
            , io_msdeque(std::move(other.io_msdeque))
            , remote(std::move(other.remote))
            , local(std::move(other.local))
        {
        }
        virtual ~asio_session()
        {

        };
    public:
        asio_session& init()
        {
            io_context.dispatch([&]
            {
                try
                {
                    // Increase the reference count of the pointer,
                    // to avoid pointer errors in asynchronous coroutine processing,
                    // as memory deallocation must consider the lifecycle of the coroutine.
                    asio::co_spawn(io_context, [self = this->shared_from_this()] { return self->reader(); }, asio::bind_executor(io_strand, asio::detached));
                    asio::co_spawn(io_context, [self = this->shared_from_this()] { return self->writer(); }, asio::bind_executor(io_strand, asio::detached));
                }
                catch (const std::exception&)
                {
                }
            });

            // throws an exception if the socket's thread is not in the same thread as the current object io_context.
            if (std::addressof(asio::query(stream_socket.get_executor(), asio::execution::context)) != std::addressof(io_context))
            {
                throw asio::error_code(asio::error::operation_aborted);
            }

            return *this;
        }
        /**
         * @brief Asynchronously send data through the socket.
         * @param buffer - The data to be sent as a string.
         * @note If the function is called from within the `io_context` thread, it directly spawns a coroutine to send the data.
         *       Otherwise, it posts the task to the `io_context` to be executed later.
         */
        asio_session& async_send(const std::string_view& buffer)
        {
            asio::co_spawn(io_context,
                           async_send_coro(buffer),
                           asio::bind_executor(io_strand, asio::detached));
            return *this;
        }

        /**
         * @brief Asynchronously write data to the socket.
         * @param buffer - The data to be written as a string.
         * @note If the function is called from within the `io_context` thread and the socket is open,
         *       the data is added to the message queue and a sleep timer is canceled to trigger immediate processing.
         *       Otherwise, it posts the task to the `io_context` to be executed later.
         */
        asio_session& async_writer(const std::string_view& buffer)
        {
            if (io_context.running_in_this_thread())
            {
                if (stream_socket.is_open())
                {
                    io_msdeque.push_back(buffer);
                    sleep->cancel_one();
                }
            }
            else
            {
                io_context.dispatch([&] { this->async_writer(buffer); });
            }

            return *this;
        }

        std::size_t index() const
        {
            return id;
        }
        /**
        * @brief Close the socket and clean up resources.
        * @note If the function is called from within the `io_context` thread, it directly closes the socket and cancels the sleep timer.
        *       Otherwise, it posts the task to the `io_context` to be executed later.
        */
        void close()
        {
            asio::error_code ec;

            try
            {
                if (io_context.running_in_this_thread())
                {
                    if (sleep->cancel() && stream_socket.is_open())
                    {
                        stream_socket.shutdown(asio::socket_base::shutdown_both, ec);
                        stream_socket.close(ec);
                    }
                }
                else
                {
                    io_context.dispatch([&] { this->close(); });
                }
            }
            catch (const std::exception&)
            {
                // Exception handling (e.g., logging) can be added here.
            }
        }

        void async_close()
        {
            try
            {
                io_context.post(std::bind(&asio_session::close, this));
            }
            catch (const std::exception&)
            {
                // Exception handling (e.g., logging) can be added here.
            }
        }

        /**
         * @brief Coroutine to asynchronously send data through the socket.
         * @param buffer - The data to be sent as a string.
         * @note This coroutine sends the data and closes the socket if an error occurs.
         */
        asio::awaitable<void> async_send_coro(const std::string_view& buffer)
        {
            asio::error_code ec;
            size_t n = 0;

            try
            {
                if (stream_socket.is_open())
                {
                    if ((n = co_await stream_socket.async_send(
                        asio::buffer(buffer.data(), buffer.size()),
                        asio::bind_executor(io_strand, asio::redirect_error(asio::use_awaitable, ec)))) < 0 || ec)
                    {
                        this->close();
                    }

                    co_await binder.async_notify(bind_type::send, io_context, self, n, ec);
                }
            }
            catch (const std::exception&)
            {
                // Exception handling (e.g., logging) can be added here.
            }
        }

        /**
         * @brief Coroutine to asynchronously read data from the socket.
         * @note This coroutine continuously reads data from the socket and notifies the binder of received data or disconnection.
         */
        asio::awaitable<void> reader()
        {
            asio::error_code ec;
            char data[8 * 1024] = { 0 };

            try
            {
                for (size_t n = 0; stream_socket.is_open();  asio::co_spawn(io_context.get_parent().get_executor(), [&] () -> asio::awaitable<void> {
                    co_await binder.async_notify(bind_type::disconnect, io_context, self, ec);
                }, asio::detached))
                {
                    for (;; co_await binder.async_notify(bind_type::recv, io_context, self, std::ref(data), n))
                    {
                        if ((n = co_await stream_socket.async_read_some(asio::buffer(std::ref(data), sizeof(data)),
                                                                        asio::bind_executor(io_strand, asio::redirect_error(asio::use_awaitable, ec)))) == 0 || ec)
                        {
                            if (ec)
                            {
                                this->close();
                                break;
                            }
                        }
                    }
                }
            }
            catch (const std::exception&)
            {
                // Exception handling (e.g., logging) can be added here.
            }
        }

        /**
         * @brief Coroutine to asynchronously write data to the socket.
         * @note This coroutine continuously writes data from the message queue to the socket.
         *       If the queue is empty, it waits for new data or a signal to proceed.
         */
        asio::awaitable<void> writer()
        {
            asio::error_code ec;

            try
            {
                for (; stream_socket.is_open(); co_await sleep->async_wait(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::duration::max()), asio::bind_executor(io_strand, asio::redirect_error(asio::use_awaitable, ec))))
                {
                    for (size_t n = 0; !io_msdeque.empty();)
                    {
                        if ((n = co_await stream_socket.async_send(asio::buffer(io_msdeque.front()),
                                                                   asio::bind_executor(io_strand, asio::redirect_error(asio::use_awaitable, ec)))) == 0 && ec)
                        {
                            this->close();
                            co_return;
                        }
                        else
                        {
                            io_msdeque.pop_front();
                        }

                        co_await binder.async_notify(bind_type::writer, io_context, self, n, ec);
                    }
                }
            }
            catch (std::exception& ex)
            {
                printf("%s\n", ex.what());
            }
        }
    private:
        asio_session&                                  self;
        asio_context&                                  io_context;
        asio::strand<asio::io_context::executor_type>  io_strand;
        asio_binder&                                   binder;
        asio_socket                                    stream_socket;
        std::size_t                                    id;
        std::shared_ptr<asio_sleep>                    sleep;
        std::deque<std::string_view>                   io_msdeque;
        asio::ip::tcp::endpoint                        remote;
        asio::ip::tcp::endpoint                        local;
    };
}

#endif // __ASIO_CHANNEL_H__