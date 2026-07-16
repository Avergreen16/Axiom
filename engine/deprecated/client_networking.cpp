#include "client_networking.hpp"
#include "utility.hpp"
#include "gui.hpp"

bool output_error(asio::error_code& ec) {
    if(ec) {
        std::cout << "Error: " << ec.message() << std::endl;
        return true;
    }
    return false;
}

void Connection::send(std::vector<uint8_t>& buffer) {
    if(status == CONNECTION_STATUS_CONNECTED) {
        asio::mutable_buffer b = asio::buffer(buffer.data(), buffer.size());

        socket->async_send(b,
            [&](asio::error_code ec, size_t bytes_transfered) {
                if(!output_error(ec)) {
                    std::cout << "Sent " << bytes_transfered << " bytes" << std::endl;
                }
            }
        );
    }
}

void Connection::close() {
    status = CONNECTION_STATUS_DISCONNECTED;
    socket.reset();
}

void Connection::start_receive() {
    socket->async_receive(asio::buffer(recieve_buffer.data(), recieve_buffer.size()),
        [&](asio::error_code ec, size_t bytes_recieved) {
            if(ec == asio::error::connection_reset) {
                uint16_t port = socket->local_endpoint().port();
                std::cout << "Connection on port " << port << " was disconnected.";
                output_error(ec);
                close();
            } else {
                if(!ec) {
                    std::cout << "Recieved " << bytes_recieved << " bytes\n";
                    std::vector<uint8_t> buffer(recieve_buffer.begin(), recieve_buffer.begin() + bytes_recieved);

                    uint32_t location = 0;

                    Packet packet;
                    packet.type = read_buffer<uint32_t>(location, buffer);
                    packet.body = std::vector<uint8_t>(buffer.begin() + location, buffer.end());

                    networking_core.packet_mutex.lock();
                    networking_core.packet_buffer_back.emplace_back(std::move(packet));
                    networking_core.packet_mutex.unlock();
                }
                start_receive();
            }
        }
    );
}

Connection::Connection(tcp::socket&& socket_) : socket(std::move(socket_)) {

}

Connection::~Connection() {
    asio::error_code ec;
    socket->close(ec);
}

bool Networking_core::is_valid(std::string ip) {
    bool valid = false;
    
    if(ip.size() >= 13) {
        if(ip[8] == ':') {
            std::string ip_address(ip.begin(), ip.begin() + 8);
            std::string ip_port(ip.begin() + 9, ip.begin() + 13);

            bool ip_valid = true;
            for(char c : ip_address) {
                if(integers_letters.find(c) == std::string::npos) {
                    ip_valid = false;
                    break;
                }
            }
            
            bool port_valid = true;
            for(char c : ip_port) {
                if(integers_letters.find(c) == std::string::npos) {
                    port_valid = false;
                    break;
                }
            }

            valid = ip_valid && port_valid;
        }
    }

    return valid;
}

std::pair<std::string, uint16_t> convert_ip(std::string ip) {
    std::string ip_address(ip.begin(), ip.begin() + 8);
    std::string ip_port(ip.begin() + 9, ip.begin() + 13);

    uint16_t port = from_base(ip_port, 16);

    std::string address;

    for(int i = 0; i < 4; ++i) {
        std::string p(ip_address.begin() + i * 2, ip_address.begin() + (i + 1) * 2);

        uint16_t num = from_base(p, 16);

        address += std::to_string(num);
        if(i != 3) address += '.';
    }

    return {address, port};
}
    
void Networking_core::connect(std::string n) {
    if(is_valid(n)) {
        auto address = convert_ip(n);

        connection.endpoint = tcp::endpoint(make_address(address.first, ec), address.second);

        connection.status = CONNECTION_STATUS_CONNECTING;
        
        connection.socket.emplace(context);
        connection.socket->open(tcp::v4());
        
        std::thread context_thread = std::thread(
            [&]() {
                asio::io_context::work idle_work(context);

                context.run();
            }
        );

        connection.socket->async_connect(connection.endpoint,
            [this, n](const std::error_code& ec) {
                if(!ec) {
                    connection.status = CONNECTION_STATUS_CONNECTED;

                    std::string name(n.begin() + 14, n.end());
                    std::vector<uint8_t> buffer;
                    write_buffer(uint32_t(0xFFFFFFFF), buffer);
                    write_buffer(name.data(), name.size(), buffer);
                    connection.send(buffer);

                    /*
                    std::thread send_thread(
                        [&]() {
                            while(true) {

                                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                            }
                        }
                    );
                    */

                    //send_thread.detach();]

                    connection.start_receive();
                } else {
                    connection.status = CONNECTION_STATUS_DISCONNECTED;
                    std::cout << "ERROR: socket failed to connect\n";
                }
            }
        );
        
        context_thread.detach();
    } else {
        std::cout << "ERROR: invalid IP\n";
    }
}

void Networking_core::call() {
    if(core.pressed_buttons.contains(GLFW_KEY_F1)) connection.status = CONNECTION_STATUS_CONNECTED;

    packet_mutex.lock();
    packet_buffer_front = std::move(packet_buffer_back);
    packet_buffer_back.clear();
    packet_mutex.unlock();
    
    for(Packet& p : packet_buffer_front) {
        if(p.type == 0) handle_packet_chat_message(p.body);
    }
}

Networking_core networking_core;

void Networking_core::handle_packet_chat_message(std::vector<uint8_t>& buffer) {
    uint32_t loc = 0;
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    while(loc < buffer.size()) {
        /*
        Chat_message message;

        // sender

        uint32_t message_size = read_buffer<uint32_t>(loc, buffer);
        std::string s;
        s.resize(message_size);
        read_buffer(s.data(), s.size(), loc, buffer);

        message.sender = s;

        // message

        message_size = read_buffer<uint32_t>(loc, buffer);
        s.resize(message_size);

        read_buffer(s.data(), s.size(), loc, buffer);

        message.message = s;

        gui_system.chat.messages.push_back(message);
        */
    }
}