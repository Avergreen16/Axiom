#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <functional>

#include "packet.hpp"

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

bool output_error(error_code& ec);

struct Time {
    steady_clock::time_point last_time;

    int hexonds_since_midnight();

    void overwrite();

    Time();

    double get_elapsed_time(bool overwrite = false);
};

struct Player_data {
    uint64_t connection;
    std::string name;
};

struct Connection {
    Player_data data;
    uint64_t id;

    tcp::socket socket;
    bool is_disconnected = false;
    std::vector<uint8_t> recieve_buffer = std::vector<uint8_t>(65536);

    void send(mutable_buffer& b);

    void start_receive();

    Connection(tcp::socket&& socket_);

    Connection(const Connection& c) = delete;

    Connection& operator=(const Connection& c) = delete;

    Connection(Connection&& c) noexcept = default;

    Connection& operator=(Connection&& c) noexcept = default;

    ~Connection();
};

struct Networking_core {
    std::atomic<uint64_t> next_connection_id = 0;

    std::unordered_map<uint64_t, std::unique_ptr<Connection>> connections;
    std::mutex delete_queue_mutex;
    std::vector<uint64_t> delete_queue;
    
    std::mutex packet_mutex;
    std::vector<std::pair<uint64_t, Packet>> packet_buffer_front;
    std::vector<std::pair<uint64_t, Packet>> packet_buffer_back;

    Time time;
    
    void broadcast(std::vector<uint8_t>& buf);

    void handle_packet_initialize(uint64_t origin, std::vector<uint8_t>& data);

    void handle_packet_message(uint64_t origin, std::vector<uint8_t>& data);

    void update();

    void create_connection(tcp::socket&& socket);
};


void accept(tcp::acceptor& acceptor, Networking_core& networking_core);

extern Networking_core networking_core;