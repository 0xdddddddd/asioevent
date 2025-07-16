#include "asio_event.hpp"
#include <iostream>
#include <memory>
#include <iostream>
#include "asio/asio_session.hpp"
using namespace ik;
//asio::awaitable<void> my_coroutine()
//{
//    // 直接使用当前 executor 构造 timer
//    asio::steady_timer timer(co_await asio::this_coro::executor,
//                             asio::chrono::seconds(1));
//    asio_sleep timer1(co_await asio::this_coro::executor);
//    co_await timer.async_wait(asio::use_awaitable);
//    std::cout << "Timer expired!" << std::endl;
//}

int main()
{
    

    asio_context io_context;
    asio_binder binder;

    //asio_binder binder;
    //asio::ip::tcp::socket socket(io_context);
    //asio_session session(io_context, binder, socket, 0);
    //
    asio::any_io_executor any_ex;
    
    asio_tcp_server server(io_context, binder);
    server.start();
    io_context.run();

    return 0;
}