#ifndef __ASIO_TRAITS_H__
#define __ASIO_TRAITS_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#	pragma once
#endif

#include <type_traits>

namespace ik
{
    template <typename T, typename = void>
    struct asio_timer_traits
    {
        using type = T;
    };

    template <typename T>
    struct asio_timer_traits<T, std::enable_if_t<std::is_same_v<T, asio::steady_timer>>>
    {
        using type = std::chrono::steady_clock::duration;
    };

    template <typename T>
    struct asio_timer_traits<T, std::enable_if_t<std::is_same_v<T, asio::system_timer>>>
    {
        using type = std::chrono::system_clock::duration;
    };
}

#endif // __ASIO_TRAITS_H__