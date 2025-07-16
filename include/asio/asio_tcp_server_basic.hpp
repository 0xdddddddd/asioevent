#ifndef __ASIO_TCP_SERVER_BASIC_H__
#define __ASIO_TCP_SERVER_BASIC_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#	pragma once
#endif

#include "asio_context.hpp"
#include "asio_context_thread_pool.hpp"
#include "asio_observer.hpp"

#include <asio.hpp>
#include <memory>
#include <atomic>

namespace ik
{
    class asio_tcp_server_basic : public std::enable_shared_from_this<asio_tcp_server_basic>
    {
    public:
        explicit asio_tcp_server_basic(asio_context& io_context, asio_binder& binder)
            : io_context(io_context)
            , binder(binder)
            , acceptor(io_context)
            , io_group(io_context)
            , io_strand(io_context.get_executor())
            , flag(1)
        {

        }
        virtual ~asio_tcp_server_basic() = default;
    public:
        /**
         * @brief Initialize the TCP server with a specified number of I/O contexts and threads.
         * @param ctx_cnt - The number of I/O contexts to initialize.
         * @param thrd_cnt - The number of threads to initialize.
         * @return Returns a reference to the current `asio_tcp_server_basic` object to support chaining.
         * @note This function initializes the I/O group with the specified number of contexts and threads.
         */
        asio_tcp_server_basic& init(std::size_t ctx_cnt, std::size_t thrd_cnt = 0)
        {
            io_group.init(ctx_cnt, thrd_cnt);
            return *this;
        }

        /**
         * @brief Add an event handler for a specific event type.
         * @param e - The event type to bind the handler to.
         * @param val - The handler function to be invoked when the event occurs.
         * @return Returns a reference to the current `asio_tcp_server_basic` object to support chaining.
         * @note This function binds a handler to a specific event type, which will be invoked when the event occurs.
         */
        template <typename F>
        asio_tcp_server_basic& add(bind_type e, F&& val)
        {
            binder.add(e, std::forward<F>(val));
            return *this;
        };

        /**
         * @brief Start the TCP server to accept incoming connections on a specified port.
         * @param port - The port number to listen on. Default is 0, which means the OS will assign a port.
         * @return Returns a reference to the current `asio_tcp_server_basic` object to support chaining.
         * @note This function spawns a coroutine to handle the asynchronous acceptance of connections.
         */
        asio_tcp_server_basic& async_listen(std::uint16_t port = 0)
        {
            return async_listen(asio::ip::tcp::endpoint(asio::ip::tcp::v6(), port));
        }

        /**
         * @brief Start the TCP server to accept incoming connections on a specified address and port.
         * @param address - The IP address to listen on.
         * @param port - The port number to listen on. Default is 0, which means the OS will assign a port.
         * @return Returns a reference to the current `asio_tcp_server_basic` object to support chaining.
         * @note This function spawns a coroutine to handle the asynchronous acceptance of connections.
         */
        asio_tcp_server_basic& async_listen(const std::string_view& address, std::uint16_t port = 0)
        {
            return async_listen(asio::ip::tcp::endpoint(asio::ip::make_address(address), port));
        }

        /**
         * @brief Start the TCP server to accept incoming connections using a specified protocol and port.
         * @param protocol - The protocol (IPv4 or IPv6) to use for the connection.
         * @param port - The port number to listen on. Default is 0, which means the OS will assign a port.
         * @return Returns a reference to the current `asio_tcp_server_basic` object to support chaining.
         * @note This function spawns a coroutine to handle the asynchronous acceptance of connections.
         */
        asio_tcp_server_basic& async_listen(const asio::ip::tcp& protocol, std::uint16_t port = 0)
        {
            return async_listen(asio::ip::tcp::endpoint(protocol, port));
        }

        /**
         * @brief Start the TCP server to accept incoming connections on a specified endpoint.
         * @param endpoint - The endpoint (IP address and port) to listen on.
         * @return Returns a reference to the current `asio_tcp_server_basic` object to support chaining.
         * @note This function spawns a coroutine to handle the asynchronous acceptance of connections.
         */
        asio_tcp_server_basic& async_listen(const asio::ip::tcp::endpoint& endpoint)
        {
            asio::co_spawn(io_context, [self = shared_from_this(), endpoint] () -> asio::awaitable<void>
            {
                co_await self->listen(endpoint);
            }, asio::bind_executor(io_strand, asio::detached));
            return *this;
        }
    private:
        /**
         * @brief Coroutine to asynchronously listen for incoming connections on a specified endpoint.
         * @param endpoint - The endpoint (IP address and port) to listen on.
         * @return Returns an `asio::awaitable<void>` that completes when the acceptor is closed.
         * @note This coroutine continuously listens for incoming connections and notifies the binder of initialization and stop events.
         *       It also handles exceptions that may occur during the listening process.
         *       The loop continues as long as the `flag` is set, and it stops when the acceptor is closed or an error occurs.
         */
        asio::awaitable<void> listen(const asio::ip::tcp::endpoint& endpoint)
        {
            try
            {
                for (acceptor.open(endpoint.protocol()),
                     acceptor.bind(endpoint),
                     acceptor.listen(),
                     co_await binder.async_notify(bind_type::init, acceptor); acceptor.is_open(); co_await binder.async_notify(bind_type::stop, acceptor))
                {
                    for (; flag.load(); )
                    {
                        co_await async_accept(io_group.get_context());
                    }
                }
            }
            catch (const std::exception& ec)
            {
                // Log or handle exceptions that occur during connection acceptance.
                printf("%s\n", ec.what());
            }
        }

        /**
         * @brief Coroutine to asynchronously accept an incoming connection.
         * @param context - The I/O context used to manage the socket.
         * @return Returns an `asio::awaitable<void>` that completes when the connection is accepted or an error occurs.
         * @note This coroutine attempts to accept a new connection. If successful, it notifies the binder of the new connection.
         *       If an error occurs during acceptance, the socket is closed, and the loop continues.
         *       If the acceptor is closed, the `flag` is reset to stop the listening loop.
         */
        asio::awaitable<void> async_accept(asio_context& context)
        {
            asio::error_code ec;
            asio::ip::tcp::socket socket(context);

            try
            {
                if (co_await acceptor.async_accept(socket, asio::bind_executor(io_strand, asio::redirect_error(asio::use_awaitable, ec))), ec)
                {
                    co_return socket.close();
                }

                if (acceptor.is_open() && socket.is_open())
                {
                    co_await binder.async_notify(bind_type::accept, context, asio_session(context, binder, socket, index.fetch_add(1)), ec);
                }
                else
                {
                    flag.exchange(0);
                }
            }
            catch (const std::exception& ec)
            {
                // Log or handle exceptions that occur during connection acceptance.
                printf("%s\n", ec.what());
            }
        }
    public:
        /**
         * @brief Stop the TCP server by closing the acceptor.
         * @return Returns a reference to the current `asio_tcp_server_basic` object to support chaining.
         * @note If called from within the `io_context` thread, the acceptor is closed directly.
         *       Otherwise, the task is posted to the `io_context` to be executed later.
         */
        asio_tcp_server_basic& stop()
        {
            if (io_context.running_in_this_thread())
            {
                flag.exchange(0);
                acceptor.close();
            }
            else
            {
                io_context.post([&] { this->stop(); });
            }

            return *this;
        }
    private:
        asio_context&                                   io_context;
        asio_binder&                                    binder;
        asio::ip::tcp::acceptor                         acceptor;
        asio_context_thread_pool                        io_group;
        asio::strand<asio::io_context::executor_type>   io_strand;
        std::atomic_size_t                              flag;
        std::atomic_size_t                              index;
    };
}

#endif // __ASIO_TCP_SERVER_BASIC_H__