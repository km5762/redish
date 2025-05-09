//
// Created by d4wgr on 5/3/2025.
//

#include "server.h"

#include <cassert>
#include <format>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <system_error>
#include <iostream>

#include "requests.h"
#include "resp.h"
#include "tcp_streambuf.h"
#include "thread_pool.h"

void Server::handle_client(const int socket) {
    TcpStreambuf tcp_streambuf{socket};
    std::iostream stream{&tcp_streambuf};

    while (auto message = resp::decode(stream)) {
        requests::handle(*message, stream, m_dictionary);
    }

    close(socket);
}


void Server::start(const std::string_view port, const int backlog_size) {
    addrinfo hints{};
    addrinfo *result{}; // will point to the results

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    if (getaddrinfo(nullptr, port.data(), &hints, &result) != 0) {
        throw std::system_error(errno, std::system_category(), "Server::start getaddrinfo");
    }

    m_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (m_socket < 0) {
        throw std::system_error(errno, std::system_category(), "Server::start socket");
    }

    constexpr int yes = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        throw std::system_error(errno, std::system_category(), "Server::start setsockopt SO_REUSEADDR");
    }

    if (bind(m_socket, result->ai_addr, result->ai_addrlen) != 0) {
        throw std::system_error(errno, std::system_category(), "Server::start bind");
    };

    freeaddrinfo(result);

    if (listen(m_socket, backlog_size) != 0) {
        throw std::system_error(errno, std::system_category(), "Server::start listen");
    }

    sockaddr_storage client_address{};
    socklen_t client_address_length{sizeof(client_address)};
    ThreadPool thread_pool{};
    while (true) {
        const int client_socket{
            accept(m_socket, reinterpret_cast<struct sockaddr *>(&client_address), &client_address_length)
        };

        if (client_socket < 0) {
            std::cerr << std::format("Warning: accept failed: {}\n", std::system_category().message(errno));
            continue;
        }

        thread_pool.enqueue([client_socket, this] {
            handle_client(client_socket);
        });
    }
}
