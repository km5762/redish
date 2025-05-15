//
// Created by d4wgr on 5/10/2025.
//

#ifndef CONNECTION_H
#define CONNECTION_H

#include <array>
#include <unistd.h>
#include <vector>

#include "event_loop.h"
#include "request_handler.h"
#include "resp_parser.h"

class RequestHandler;

class Connection final : public EventLoop::Handler {
public:
    Connection(const int socket, RequestHandler &request_handler, EventLoop &event_loop): m_request_handler{
            request_handler
        },
        m_event_loop{event_loop}, m_socket{socket} {
    }

    ~Connection() override {
        close(m_socket);
    }

    void handle(uint32_t events) override;

    void send(const resp::Value &value);

private:
    static constexpr int buffer_size{4096};
    resp::Parser m_parser{};
    RequestHandler &m_request_handler;
    EventLoop &m_event_loop;
    int m_socket{};
    std::array<char, buffer_size> m_read_buffer{};
    std::vector<char> m_write_buffer{};
    size_t m_bytes_sent{0};
    size_t m_bytes_remaining{0};

    void handle_receive();

    void handle_send();
};


#endif //CONNECTION_H
