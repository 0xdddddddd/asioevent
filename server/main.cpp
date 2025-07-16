#include "asio_event.hpp"

int main()
{

    using namespace ik;
    asio_context io_context;
    asio_binder binder;
    asio_tcp_server server(io_context, binder);
    server.start();
    io_context.run();
    return 0;
}