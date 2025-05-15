//
// Created by d4wgr on 5/14/2025.
//

#ifndef ACCEPTOR_H
#define ACCEPTOR_H
#include <unistd.h>

#include "event_loop.h"
#include "request_handler.h"


class Acceptor final : public EventLoop::Handler {
public:
    Acceptor(const int socket, EventLoop &event_loop, RequestHandler &request_handler): m_socket{socket},
        m_event_loop{event_loop}, m_request_handler{request_handler} {
    }

    ~Acceptor() override {
        close(m_socket);
    }

    void handle(uint32_t events) override;

private:
    int m_socket{-1};
    EventLoop &m_event_loop;
    RequestHandler &m_request_handler;
};


#endif //ACCEPTOR_H
