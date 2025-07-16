#ifndef __ASIO_UTILS_H__
#define __ASIO_UTILS_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#	pragma once
#endif

#include "asio_traits.hpp"

#include <iostream>

namespace ik
{
    using asio_acceptor = asio::ip::tcp::acceptor;
    using asio_socket = asio::ip::tcp::socket;
    using asio_endpoint = asio::ip::tcp::endpoint;
    using asio_resolver = asio::ip::tcp::resolver;

    using asio_error = asio::error_code;

    struct asio_buf_t
    {
        char* data;
        std::size_t n;
    };
}
#endif