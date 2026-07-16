#pragma once

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#include "packet.hpp"

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
#define ASIO_STANDALONE
#include "asio.hpp"
#include "asio/ts/buffer.hpp"
#include "asio/ts/internet.hpp"

using namespace std::chrono;
using namespace asio::ip;

const uint16_t server_port = 0xEEEE;

bool output_error(asio::error_code& ec);

enum connection_status{CONNECTION_STATUS_DISCONNECTED, CONNECTION_STATUS_CONNECTING, CONNECTION_STATUS_CONNECTED};

struct Connection {
    tcp::endpoint endpoint;
    std::optional<tcp::socket> socket;
    connection_status status = CONNECTION_STATUS_DISCONNECTED;
    std::vector<uint8_t> recieve_buffer = std::vector<uint8_t>(65536);

    void send(std::vector<uint8_t>& b);

    void start_receive();

    void close();

    Connection(tcp::socket&& socket_);

    Connection() = default;

    Connection(const Connection& c) = delete;

    Connection& operator=(const Connection& c) = delete;

    Connection(Connection&& c) noexcept = default;

    Connection& operator=(Connection&& c) noexcept = default;

    ~Connection();
};

struct Networking_core {
    asio::error_code ec;
    asio::io_context context;
    Connection connection;

    std::mutex packet_mutex;
    std::vector<Packet> packet_buffer_front;
    std::vector<Packet> packet_buffer_back;

    bool is_valid(std::string ip);

    void connect(std::string i);

    void call();

    void handle_packet_chat_message(std::vector<uint8_t>& buffer);
};

extern Networking_core networking_core;