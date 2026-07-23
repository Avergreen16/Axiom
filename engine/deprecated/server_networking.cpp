#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <functional>

#include "server_networking.hpp"

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
#define ASIO_STANDALONE
#include "asio/asio.hpp"
#include "asio/ts/buffer.hpp"
#include "asio/ts/internet.hpp"

using namespace std::chrono;
using namespace asio::ip;
using namespace asio;

const double hexond_ratio = 86400.0 / 65536.0;

bool output_error(error_code& ec) {
    if(ec) {
        std::cout << "Error: " << ec.message() << std::endl;
        return true;
    }
    return false;
}

int Time::hexonds_since_midnight() {
    using namespace std::chrono;

    // Current time
    auto now = system_clock::now();

    // Convert to local time
    auto local = zoned_time{current_zone(), now};

    // Get today's date at midnight
    auto today = floor<days>(local.get_local_time());

    // Time since midnight
    auto since_midnight = local.get_local_time() - today;

    double sec = since_midnight.count() / 1000000000.0;

    return sec / hexond_ratio;
}

void Time::overwrite() {
    last_time = steady_clock::now();
}

Time::Time() {
    overwrite();
}

double Time::get_elapsed_time(bool overwrite = false) {
    steady_clock::time_point current_time = steady_clock::now();
    steady_clock::duration duration = current_time - last_time;

    if(overwrite) {
        last_time = current_time;
    }

    return double(duration.count()) * steady_clock::period::num / steady_clock::period::den;
}

//

Connection::Connection(tcp::socket&& socket_) : socket(std::move(socket_)) {

}

Connection::~Connection() {
    error_code ec;
    socket.close(ec);
}

void Connection::start_receive() {
    socket.async_receive(buffer(recieve_buffer.data(), recieve_buffer.size()),
        [&](error_code ec, size_t bytes_recieved) {
            if(ec == error::connection_reset) {
                uint16_t port = socket.local_endpoint().port();
                std::cout << "Connection on port " << port << " was disconnected. ";
                output_error(ec);
                is_disconnected = true;

                networking_core.delete_queue_mutex.lock();
                networking_core.delete_queue.push_back((uint64_t)this);
                networking_core.delete_queue_mutex.unlock();
            } else {
                if(!output_error(ec)) {
                    std::cout << "Recieved " << bytes_recieved << " bytes\n";

                    std::vector<uint8_t> buffer(recieve_buffer.begin(), recieve_buffer.begin() + bytes_recieved);

                    uint32_t location = 0;

                    Packet packet;
                    packet.type = read_buffer<uint32_t>(location, buffer);
                    packet.body = std::vector<uint8_t>(buffer.begin() + location, buffer.end());

                    networking_core.packet_mutex.lock();
                    networking_core.packet_buffer_back.emplace_back(std::move(std::pair<uint64_t, Packet>(id, std::move(packet))));
                    networking_core.packet_mutex.unlock();
                }
                start_receive();
            }
        }
    );
}

void Connection::send(mutable_buffer& b) {
    if(!is_disconnected) {
        socket.async_send(b,
            [&](error_code ec, size_t bytes_transfered) {
                if(!output_error(ec)) {
                    std::cout << "Sent " << bytes_transfered << " bytes" << std::endl;
                }
            }
        );
    }
}

//

    
void Networking_core::broadcast(std::vector<uint8_t>& buf) {
    mutable_buffer b = buffer(buf.data(), buf.size());

    for(auto& [key, c] : connections) {
        c.get()->send(b);
    }
}

void Networking_core::handle_packet_initialize(uint64_t origin, std::vector<uint8_t>& data) {
    if(connections.contains(origin)) {
        auto& connection = connections[origin];

        uint32_t loc;
        std::string name;
        name.resize(data.size());
        read_buffer(name.data(), name.size(), loc, data);

        connection->data.name = name;
    }
}

void Networking_core::handle_packet_message(uint64_t origin, std::vector<uint8_t>& data) {
    auto& connection = connections[origin];

    std::string origin_name = connection->data.name;
    std::string message;

    uint32_t loc;
    message.resize(data.size());
    read_buffer(message.data(), message.size(), loc, data);
    

    std::vector<uint8_t> buf;
    write_buffer(uint32_t(0), buf);
    write_buffer(uint32_t(origin_name.size()), buf);
    write_buffer(origin_name.data(), origin_name.size(), buf);
    write_buffer(uint32_t(message.size()), buf);
    write_buffer(message.data(), message.size(), buf);
    
    broadcast(buf);
}

void Networking_core::update() {
    delete_queue_mutex.lock();
    for(uint16_t p : delete_queue) {
        connections.erase(p);
    }
    delete_queue.clear();
    delete_queue_mutex.unlock();

    if(time.get_elapsed_time() > 5.0 && false) {
        std::string origin_name = "\\cF66SERVER";
        std::string message = "\\cF66Hello, client!";

        std::vector<uint8_t> buf;
        write_buffer(uint32_t(0), buf);
        write_buffer(uint32_t(origin_name.size()), buf);
        write_buffer(origin_name.data(), origin_name.size(), buf);
        write_buffer(uint32_t(message.size()), buf);
        write_buffer(message.data(), message.size(), buf);
        
        broadcast(buf);
        time.overwrite();
    }

    packet_mutex.lock();
    packet_buffer_front = std::move(packet_buffer_back);
    packet_buffer_back.clear();
    packet_mutex.unlock();
    
    for(auto& [origin, p] : packet_buffer_front) {
        if(p.type == 0xFFFFFFFF) handle_packet_initialize(origin, p.body);
        else if(p.type == 0x0) handle_packet_message(origin, p.body);
    }
}

void Networking_core::create_connection(tcp::socket&& socket) {
    std::unique_ptr<Connection> ptr = std::make_unique<Connection>(Connection(std::move(socket)));
    ptr.get()->start_receive();
    uint64_t index = next_connection_id++;

    ptr.get()->id = index;
    
    connections.emplace(index, std::move(ptr));
}

//

void accept(tcp::acceptor& acceptor, Networking_core& networking_core) {
    acceptor.async_accept(
        [&](error_code ec, tcp::socket&& socket) {
            if(!ec) {
                std::cout << "Client connected" << std::endl;

                networking_core.create_connection(std::move(socket));
            } else {
                std::cout << "Error: " << ec.message() << std::endl;
            }
            accept(acceptor, networking_core);
        }
    );
}

//

Networking_core networking_core;