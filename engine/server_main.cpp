#include "server_networking.hpp"

const uint16_t port = 0xEEEE;

int main() {
    asio::error_code ec;

    io_context context;

    io_context::work work(context);

    std::thread context_thread(
        [&]() {
            context.run();
        }
    );

    tcp::endpoint endpoint(tcp::v4(), port);

    tcp::acceptor acceptor(context, endpoint);
    
    accept(acceptor, networking_core);

    while(true) {
        networking_core.update();
    }
}