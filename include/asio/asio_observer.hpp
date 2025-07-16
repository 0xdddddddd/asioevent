#ifndef __ASIO_OBSERVICE_H__
#define __ASIO_OBSERVICE_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#	pragma once
#endif

#include <iostream>
#include <type_traits>
#include <functional>
#include <unordered_map>
#include <execution>

#include <asio.hpp>

namespace ik
{
    /**
     * @brief Enumeration type for identifying different event types
     * @tparam std::size_t The underlying type of the enumeration is std::size_t
     */
    enum class bind_type : std::size_t
    {
        /**
         * @brief Initialization event
         * @note Typically triggered when the server or client starts, used to perform initialization tasks
         * @example
         * binder.add(bind_type::init, [&] (asio_event& context) {
         *     // Perform initialization tasks here
         * });
         */
        init,

        /**
         * @brief Stop event
         * @note Typically triggered when the server or client shuts down, used to perform cleanup tasks
         * @example
         * binder.add(bind_type::stop, [&] (asio_event& context) {
         *     // Perform cleanup tasks here
         * });
         */
        stop,

        /**
         * @brief Data receive event
         * @note Triggered when data is received from a client, used to process the received data
         * @example
         * binder.add(bind_type::recv, [&] (asio_event& context, asio_session& session, const char* buf, size_t n, asio_error& ec) {
         *     // Process the received data (buf) of size (n)
         *     // Example: Echo the data back to the client
         *     session.async_send(buf, n);
         * });
         */
        recv,

        /**
         * @brief Data send event
         * @note Triggered when data is successfully sent to a client, used to confirm the data has been sent
         * @example
         * binder.add(bind_type::send, [&] (asio_event& context, asio_session& session, size_t n, asio_error& ec) {
         *     // Handle the result of the send operation
         *     if (!ec) {
         *         printf("Sent %zu bytes\n", n); // Print the number of bytes sent
         *     }
         * });
         */
        send,

        /**
         * @brief Write operation completion event
         * @note Typically used as a callback when an asynchronous write operation completes, to handle the result of the write operation
         * @example
         * binder.add(bind_type::writer, [&] (asio_event& context, asio_session& session, asio_error& ec) {
         *     if (!ec) {
         *         // Handle successful write operation
         *     }
         * });
         */
        writer,

        /**
         * @brief Connection event
         * @note Triggered when a new client connects, used to create a new session or handle the new connection
         * @example
         * binder.add(bind_type::connect, [&] (asio_event& context, asio_socket& socket, asio_error& ec, std::size_t ider) {
         *     sessions.emplace(ider, std::make_shared<asio_session>(io_context, binder, socket, ider)); // Store the new session
         * });
         */
        connect,
        connect_timeout,

        /**
         * @brief Disconnection event
         * @note Triggered when a client disconnects, used to clean up the session or handle post-disconnection logic
         * @example
         * binder.add(bind_type::disconnect, [&] (asio_event& context, asio_session& session, asio_error& ec) {
         *     // Remove the session from the session map
         *     sessions.erase(session.index());
         * });
         */
        disconnect,

        /**
         * @brief Accept connection event
         * @note Triggered when the server accepts a new connection, used to handle the initialization of the new connection
         * @example
         * binder.add(bind_type::accept, [&] (asio_event& context, asio_socket& socket, asio_error& ec) {
         *     if (!ec) {
         *         // Handle the new connection
         *     }
         * });
         */
        accept,

        /**
         * @brief Maximum value of the enumeration
         * @note Used to represent the maximum value of the enumeration, typically for iteration or boundary checking to avoid out-of-bounds errors
         * @example
         * for (std::size_t i = 0; i < static_cast<std::size_t>(bind_type::max); ++i) {
         *     // Iterate over all enumeration values
         * }
         */
        max
    };

    namespace details
    {
        template <typename T>
        struct is_lambda
        {
        private:
            template <typename U>
            static auto test(int) -> decltype(&U::operator(), std::true_type{});
            template <typename U>
            static std::false_type test(...) {};
        public:
            static constexpr bool value = std::is_class<T>::value && decltype(test<T>(0))::value;
        };

        template <typename T>
        inline constexpr bool is_lambda_v = is_lambda<T>::value;

        template <typename... Types>
        struct function_traits_type {};

        template <typename T>
        struct function_traits
        {
            using type = T;
        };

        template <typename T, typename... Types>
        struct function_traits<T(__cdecl)(Types...)>
        {
            using type = T(Types...);
        };

        template <typename T, typename... Types>
        struct function_traits<T(__stdcall)(Types...)>
        {
            using type = T(Types...);
        };

        template <typename T, typename... Types>
        struct function_traits<T(__fastcall)(Types...)>
        {
            using type = T(Types...);
        };

        template <typename T, typename... Types>
        struct function_traits<T(__vectorcall)(Types...)>
        {
            using type = T(Types...);
        };

        template <typename T, typename... Types>
        struct function_traits<T(__cdecl&)(Types...)>
        {
            using type = T(Types...);
        };

        template <typename T, typename... Types>
        struct function_traits<T(__stdcall&)(Types...)>
        {
            using type = T(Types...);
        };

        template <typename T, typename... Types>
        struct function_traits<T(__fastcall&)(Types...)>
        {
            using type = T(Types...);
        };

        template <typename T, typename... Types>
        struct function_traits<T(__vectorcall&)(Types...)>
        {
            using type = T(Types...);
        };

        template <typename T, typename C, typename... Types>
        struct function_traits<T(__thiscall C::*)(Types...)>
        {
            using type = T(Types...);
        };

        template <typename T, typename C, typename... Types>
        struct function_traits<T(__cdecl C::*)(Types...)>
        {
            using type = T(Types...);
        };

        template <typename T, typename C, typename... Types>
        struct function_traits<T(__stdcall C::*)(Types...)>
        {
            using type = T(Types...);
        };

        template <typename T, typename C, typename... Types>
        struct function_traits<T(__fastcall C::*)(Types...)>
        {
            using type = T(Types...);
        };

        template <typename T, typename C, typename... Types>
        struct function_traits<T(__vectorcall C::*)(Types...)>
        {
            using type = T(Types...);
        };

        template <typename T>
        struct function_traits_of
        {
            using type = typename function_traits<std::remove_pointer_t<T>>::type;
        };

        // lambda
        template <typename T>
        struct function_lambda_traits;

        template <typename R, typename... Args>
        struct function_lambda_traits<R(Args...)>
        {
            using type = R(Args...);
            using return_type = R;
            static constexpr std::size_t arity = sizeof...(Args);

            template <std::size_t I>
            struct arg
            {
                using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
            };
        };

        template <typename C, typename R, typename... Args>
        struct function_lambda_traits<R(C::*)(Args...) const> : function_lambda_traits<R(Args...)> {};

        template <typename C, typename R, typename... Args>
        struct function_lambda_traits<R(C::*)(Args...)> : function_lambda_traits<R(Args...)> {};

        template <typename T>
        struct function_lambda_traits_of
        {
            using type = typename function_lambda_traits<decltype(&std::remove_reference_t<T>::operator())>::type;
        };

        template <typename T, typename E = void>
        struct function_lambda_traits_if
        {
            using type = T;
        };

        template <typename T>
        struct function_lambda_traits_if<T, std::enable_if_t<is_lambda_v<T>>>
        {
            using type = typename function_lambda_traits_of<T>::type;
        };

        // std::bind
        template <typename R, typename F, typename... Types>
        using function_bind = std::_Binder<R, F, Types ...>;

        template <typename T>
        struct function_bind_traits;

        template <typename T>
        struct function_bind_traits
        {
            using type = T;
        };

        // std::bind ref
        // class std::_Binder<struct std::_Unforced,void (__cdecl&)(int,int),struct std::_Ph<1> const &,struct std::_Ph<2> const &>
        template <typename R, typename F>
        struct function_bind_traits<function_bind<R, F&>>
        {
            using type = F;
        };

        template <typename R, typename F>
        struct function_bind_traits<function_bind<R, F>>
        {
            using type = F;
        };

        // std::bind ref placeholders
        template <typename R, typename F, typename C, typename... Types>
        struct function_bind_traits<function_bind<R, F&, C, Types...>>
        {
            using type = F;
        };

        // std::bind placeholders
        template <typename R, typename F, typename C, typename... Types>
        struct function_bind_traits<function_bind<R, F, C, Types...>>
        {
            using type = F;
        };

        template <typename T>
        struct function_bind_traits_of
        {
            using type = typename std::remove_pointer_t<typename function_lambda_traits_if<typename function_bind_traits<T>::type>>::type;
        };

        template <typename T, typename E = void>
        struct function_traits_cvref
        {
            static_assert((std::is_function_v<std::remove_pointer_t<T>> || std::is_member_function_pointer_v<T>) ||
                          (!std::is_bind_expression_v<T> && is_lambda_v<T>) ||
                          (std::is_bind_expression_v<T> && !is_lambda_v<T>),
                          "Unsupported type: T must be a function, member function pointer, lambda, or bind expression.");
        };

        template <typename T>
        struct function_traits_cvref<T, std::enable_if_t<std::is_function_v<std::remove_pointer_t<T>> || std::is_member_function_pointer_v<T>>>
        {
            using type = typename function_traits_of<T>::type;
        };

        template <typename T>
        struct function_traits_cvref<T, std::enable_if_t<!std::is_bind_expression_v<T>&& is_lambda_v<T>>>
        {
            using type = typename function_lambda_traits_of<T>::type;
        };

        template <typename T>
        struct function_traits_cvref<T, std::enable_if_t<std::is_bind_expression_v<T> && !is_lambda_v<T>>>
        {
            using type = typename function_bind_traits_of<T>::type;
        };

        template <typename T>
        using function_traits_cvref_t = typename function_traits_cvref<T>::type;


        template <typename T, template <typename, typename...> typename ImplType>
        struct function_analysis {};

        // Function pointer
        template <typename T, typename... Types, template <typename, typename...> typename ImplType>
        struct function_analysis<T(__cdecl)(Types...), ImplType>
        {
            using type = ImplType<T, Types...>;
        };

        template <typename T, typename... Types, template <typename, typename...> typename ImplType>
        struct function_analysis<T(__fastcall)(Types...), ImplType>
        {
            using type = ImplType<T, Types...>;
        };

        template <typename T, typename... Types, template <typename, typename...> typename ImplType>
        struct function_analysis<T(__stdcall)(Types...), ImplType>
        {
            using type = ImplType<T, Types...>;
        };

        template <typename T, typename... Types, template <typename, typename...> typename ImplType>
        struct function_analysis<T(__vectorcall)(Types...), ImplType>
        {
            using type = ImplType<T, Types...>;
        };

        // Member function pointer
        template <typename T, typename C, typename... Types, template <typename, typename...> typename ImplType>
        struct function_analysis<T(__thiscall C::*)(Types...), ImplType>
        {
            using type = ImplType<T, Types...>;
        };

        template <typename T, typename C, typename... Types, template <typename, typename...> typename ImplType>
        struct function_analysis<T(__cdecl C::*)(Types...), ImplType>
        {
            using type = ImplType<T, Types...>;
        };

        template <typename T, typename C, typename... Types, template <typename, typename...> typename ImplType>
        struct function_analysis<T(__stdcall C::*)(Types...), ImplType>
        {
            using type = ImplType<T, Types...>;
        };

        template <typename T, typename C, typename... Types, template <typename, typename...> typename ImplType>
        struct function_analysis<T(__fastcall C::*)(Types...), ImplType>
        {
            using type = ImplType<T, Types...>;
        };

        template <typename T, typename C, typename... Types, template <typename, typename...> typename ImplType>
        struct function_analysis<T(__vectorcall C::*)(Types...), ImplType>
        {
            using type = ImplType<T, Types...>;
        };
    }

    template <typename R, typename... Types>
    struct observer_wrapper_callable_base
    {
        virtual R invoke(Types&&...) = 0;
    };

    template <typename T, typename R, typename... Types>
    class observer_wrapper_callable : public observer_wrapper_callable_base<R, Types...>
    {
    public:
        using return_type = R;
    public:
        template <typename F>
        explicit observer_wrapper_callable(F&& val) : callable(std::forward<F>(val)) {}
    public:
        return_type virtual invoke(Types&&... args) override
        {
            try
            {
                if constexpr (std::is_void_v<return_type>)
                {
                    return std::invoke(callable, std::forward<Types>(args)...);
                }
                else
                {
                    return std::invoke(callable, std::forward<Types>(args)...);
                }
            }
            catch (const std::bad_function_call& ex)
            {
                std::cerr << "Caught std::bad_function_call: " << ex.what() << std::endl;
            }
            catch (const std::exception& ex)
            {
                std::cerr << "Caught exception: " << ex.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "Caught unknown exception!" << std::endl;
            }
        }
    private:
        T callable;
    };

    class observer_base
    {
    public:
        virtual ~observer_base() noexcept = default;
    };

    template <typename R, typename... Types>
    class observer_impl : public observer_base
    {
    public:
        using callable_type = observer_wrapper_callable_base<R, Types...>;
        using argument_types = std::tuple<Types...>;
        using return_type = R;
    public:
        template <class F>
        constexpr explicit observer_impl(F&& val)
        {
            ptr = new observer_wrapper_callable<std::decay_t<F>, return_type, Types...>(std::forward<F>(val));
        }
        virtual ~observer_impl() noexcept
        {
            if (ptr) { delete ptr; }
        }
    public:
        /**
         * @brief Function call operator to invoke the stored callable object.
         * @tparam Types - The types of the arguments to pass to the callable object.
         * @param args - The arguments to pass to the callable object.
         * @return Returns the result of invoking the callable object.
         * @note This operator forwards the arguments to the `call` method.
         */
        inline return_type operator()(Types&&... args) const
        {
            return this->call(std::forward<Types>(args)...);
        }

        /**
         * @brief Invoke the stored callable object with the provided arguments.
         * @tparam Types - The types of the arguments to pass to the callable object.
         * @param args - The arguments to pass to the callable object.
         * @return Returns the result of invoking the callable object.
         * @note If the callable object is valid (i.e., `ptr` is not null), it is invoked with the forwarded arguments.
         *       Otherwise, a default-constructed `return_type` is returned.
         */
        inline return_type call(Types&&... args) const
        {
            if (ptr)
            {
                return static_cast<callable_type*>(ptr)->invoke(std::forward<Types>(args)...);
            }
            return return_type{};
        }
    private:
        void* ptr;
    };

    template <typename T>
    class observer : public details::function_analysis<T, observer_impl>::type
    {
    public:
        template <typename F>
        explicit observer(F&& val)
            : details::function_analysis<T, observer_impl>::type(std::forward<F>(val))
        {
        }
    };

    class asio_binder
    {
    public:
        /**
         * @brief Add an observer for a specific event type.
         * @tparam F - The type of the observer function or callable object.
         * @param e - The event type to bind the observer to.
         * @param val - The observer function or callable object to be invoked when the event occurs.
         * @note This function stores the observer in a map indexed by the event type.
         *       The observer is wrapped in a `std::unique_ptr` for lifetime management.
         */
        template <typename T = bind_type, typename F>
        inline asio_binder& add(T&& e, F&& val) noexcept
        {
            observers[std::forward<T>(e)] = std::make_unique<observer<typename details::function_traits_cvref<std::decay_t<F>>::type>>(std::forward<F>(val));
            return *this;
        }

        /**
        * @brief Overload the | operator to add an observer for a specific event type.
        * @tparam F - The type of the observer function or callable object.
        * @param pair - A pair of the event type and the observer function.
        * @note This function extracts the event type and observer from the pair and adds them to the observers map.
        */
        template <typename T = bind_type, typename F>
        inline asio_binder& operator|(std::pair<T, F>&& pair) noexcept
        {
            return add(std::forward<T>(pair.first), std::forward<F>(pair.second));
        }

        /**
         * @brief Remove the observer for a specific event type.
         * @param e - The event type to remove the observer from.
         * @note This function sets the observer for the specified event type to `nullptr`.
         */
        template <typename T = bind_type>
        inline asio_binder& del(T&& e)
        {
            observers[std::forward<T>(e)] = nullptr;
            return *this;
        }

        /**
         * @brief Notify the observer for a specific event type.
         * @tparam R - The return type of the observer function (default is `void`).
         * @tparam Types - The types of the arguments to pass to the observer function.
         * @param e - The event type to notify.
         * @param args - The arguments to pass to the observer function.
         * @return Returns the result of the observer function call, or a default-constructed `R` if no observer is registered.
         * @note This function checks if an observer is registered for the event type and calls it if available.
         */
        template <typename R = void, typename T = bind_type, typename... Types>
        inline auto notify(T&& e, Types&&... args) noexcept
        {
            if (observers[std::forward<T>(e)] == nullptr)
            {
                return R{};
            }

            observer_impl<R, Types...>* ptr = static_cast<observer_impl<R, Types...>*>(observers[std::forward<T>(e)].get());

            if (ptr)
            {
                return ptr->call(std::forward<Types>(args)...);
            }

            return R{};
        }

        /**
         * @brief Notify the observer for a specific event type asynchronously using a coroutine.
         * @tparam R - The return type of the observer function (default is `void`).
         * @tparam Types - The types of the arguments to pass to the observer function.
         * @param e - The event type to notify.
         * @param args - The arguments to pass to the observer function.
         * @return Returns an `asio::awaitable<R>` that resolves to the result of the observer function call,
         *         or a default-constructed `R` if no observer is registered.
         * @note This function checks if an observer is registered for the event type and calls it if available.
         *       The result is returned as a coroutine.
         */
#ifdef ASIO_DETAIL_CONFIG_HPP
        template <typename R = void, typename T = bind_type, typename... Types>
        asio::awaitable<R> async_notify(T&& e, Types&&... args) noexcept
        {
            co_return this->notify(std::forward<T>(e), std::forward<Types>(args)...);
        }
#endif
    private:
        std::unordered_map<bind_type, std::unique_ptr<observer_base>> observers;
    };
}

#endif // __ASIO_OBSERVICE_H__