#include "asio_event.hpp"


void aaaa(int a, int b)
{

}

int main()
{
    using namespace ik;
    asio_context io_context;
    std::atomic_size_t nnn = 0;

    asio_binder binder;

    //std::unordered_map<size_t, std::shared_ptr<asio_session<asio::ip::udp>>> sessions;


   /* binder.add(bind_type::connect, [&] (asio_context& context, asio_session<asio::ip::udp>& socket, asio_error& ec)
    {
        if (ec)
        {
            return;
        }

        size_t index = nnn.fetch_add(1);
        sessions.emplace(index, std::make_shared<asio_session<asio::ip::udp>>(io_context, binder, socket, index));
    })
        .add(bind_type::disconnect, [&] (asio_context& context, asio_session<asio::ip::udp>& s, asio_error& ec)
    {
        sessions.erase(s.index());
    })
        .add(bind_type::recv, [&] (asio_context& context, asio_session<asio::ip::udp>& s, const char* buf, size_t n)
    {
        s.async_send("123456\n");
    })
        .add(bind_type::send, [&] (asio_context& context, asio_session<asio::ip::udp>& s, size_t n, asio_error& ec)
    {
        printf("%3d\n", n);
    });*/

    ////binder | std::make_pair(bind_type::connect, std::bind([]{}));



    //std::unordered_map<size_t, std::shared_ptr<asio_tcp_client>> clients;

    //for (u32 i = 0; i < 1; ++i)
    //{
    //    auto client = std::make_shared<asio_tcp_client>(io_context, binder);
    //    client->async_connect("10.1.2.7", 6666);
    //    clients.emplace(i, client);
    //}


    //client.async_connect_with_resolver("www.baidu.com", "80");*/
    /*asio_event_thread_pool thread_pool(io_context);
    thread_pool.init(16);
    asio_tcp_client_pool client(io_context, thread_pool, test);
    client.init("10.1.2.10", 6666, 1);*/
    //// 运行事件循环
    io_context.run();

    return 0;
}