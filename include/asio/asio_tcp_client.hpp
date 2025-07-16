#ifndef __ASIO_TCP_CLIENT_H__
#define __ASIO_TCP_CLIENT_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#	pragma once
#endif

#include "asio_observer.hpp"
#include "asio_session.hpp"
#include "asio_utils.hpp"

namespace ik
{
    struct MyStruct
    {
        
    };

    class asio_tcp_client : public std::enable_shared_from_this<asio_tcp_client>
    {
    public:
        explicit asio_tcp_client(asio_context& io_context, asio_binder& binder)
            : io_context(io_context)
            , binder(binder)
            , io_strand(io_context.get_executor())
        {

        }
        virtual ~asio_tcp_client()
        {

        };
    public:
        /**
         * @brief Add an event handler for a specific event type.
         * @param e - The event type to bind the handler to.
         * @param val - The handler function to be invoked when the event occurs.
         * @return Returns a reference to the current `asio_tcp_client` object to support chaining.
         */
        template <typename F>
        asio_tcp_client& add(bind_type e, F&& val)
        {
            binder.add(e, std::forward<F>(val));
            return *this;
        };

        /**
         * @brief Asynchronously connect to a remote server at the specified address and port.
         * @param address - The IP address or hostname of the remote server.
         * @param port - The port number of the remote server.
         * @note If a connection is already established (i.e., `channel` is not null), this function does nothing.
         */
        asio_tcp_client& async_connect(const std::string& address, std::uint16_t port)
        {
            asio::error_code ec;

            try
            {
                return async_connect(asio_endpoint(asio::ip::make_address(address), port));
            }
            catch (const std::exception&)
            {

            }

            return *this;
        }

        asio_tcp_client& async_connect(const asio_endpoint& endpoint)
        {
            asio::error_code ec;

            try
            {
                asio::co_spawn(io_context, [self = this->shared_from_this(), endpoint] () -> asio::awaitable<void> {
                    co_await self->connect(endpoint);
                }, asio::bind_executor(io_strand, asio::redirect_error(asio::detached, ec)));
            }
            catch (const std::exception&)
            {

            }

            return *this;
        }

        /**
        * @brief Asynchronously connect to a remote server using a resolver to resolve the address and scheme.
        * @param address - The address (hostname or IP) of the remote server to connect to.
        * @param scheme - The scheme (e.g., "http", "https", or port number) used for resolving the address.
        * @note If a connection is already established (i.e., `channel` is not null), this function does nothing.
        */
        asio_tcp_client& async_connect_resolver(const std::string& hostname, const std::string& scheme)
        {
            asio::error_code ec;

            try
            {
                asio::co_spawn(io_context, [self = this->shared_from_this(), hostname, scheme] () -> asio::awaitable<void> {
                    co_await self->connect_resolver(asio_resolver::query(hostname, scheme));
                }, asio::bind_executor(io_strand, asio::redirect_error(asio::detached, ec)));
            }
            catch (const std::exception&)
            {

            }

            return *this;
        }

        /**
         * @brief Asynchronously connect to a remote endpoint using a coroutine.
         * @param endpoint - The remote endpoint to connect to.
         * @return Returns a boolean indicating whether the connection was successful.
         *         `true` if the connection was successful and `channel` is valid, otherwise `false`.
         * @note This function is a coroutine and must be awaited.
         */
        asio::awaitable<bool> connect(const asio_endpoint& endpoint)
        {
            asio::error_code ec;
            asio_socket stream_socket(io_context);

            try
            {
                if (co_await binder.async_notify(bind_type::init, io_context, stream_socket),
                    co_await stream_socket.async_connect(endpoint,
                                                         asio::bind_executor(io_strand, asio::redirect_error(asio::use_awaitable, ec))), ec)
                {
                    stream_socket.close();
                }

                co_await binder.async_notify(bind_type::connect, io_context, stream_socket, ec);
            }
            catch (const std::exception&)
            {

            }

            co_return ec.value() == 0;
        }

        /**
         * @brief Asynchronously resolve and connect to a remote address using a coroutine.
         * @param query - The resolver query containing the address and scheme to resolve.
         * @note This function is a coroutine and must be awaited.
         */
        asio::awaitable<void> connect_resolver(const asio_resolver::query& query)
        {
            asio::error_code ec;
            asio_resolver resolver(io_context);

            try
            {
                for (auto& entry : co_await resolver.async_resolve(query,
                                                                   asio::bind_executor(io_strand, asio::redirect_error(asio::use_awaitable, ec))))
                {
                    if (co_await connect(entry.endpoint()))
                    {
                        break;
                    }
                }
            }
            catch (const std::exception&)
            {

            }
        }
    private:
        asio_context&                                   io_context;
        asio_binder&                                    binder;
        asio::strand<asio::io_context::executor_type>   io_strand;
    };
}

#endif // __ASIO_TCP_CLIENT_H__