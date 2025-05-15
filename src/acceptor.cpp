//
// Created by d4wgr on 5/14/2025.
//

#include "acceptor.h"
#include "connection.h"

#include <format>
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>

void Acceptor::handle(const uint32_t events) {
    if (!(events & EPOLLIN)) {
        return;
    }


    sockaddr_storage client_address{};
    socklen_t client_address_length{sizeof(client_address)};

    const int client_socket = accept(m_socket, reinterpret_cast<struct sockaddr *>(&client_address),
                                     &client_address_length);

    if (client_socket < 0) {
        std::cerr << std::format("Warning: accept failed: {}\n", std::system_category().message(errno));
        return;
    }

    m_event_loop.add_handler(
        client_socket,
        EPOLLIN,
        std::make_unique<Connection>(client_socket,
                                     m_request_handler,
                                     m_event_loop)
    );
}
