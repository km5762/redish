//
// Created by d4wgr on 5/3/2025.
//

#include "server.h"

#include <cassert>
#include <format>
#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <system_error>
#include <sys/epoll.h>
#include <fcntl.h>
#include <fstream>
#include <bits/fs_fwd.h>

#include "acceptor.h"
#include "request_handler.h"


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

    const addrinfo *p{nullptr};
    for (p = result; p != nullptr; p = p->ai_next) {
        if ((m_socket = socket(p->ai_family, p->ai_socktype,
                               p->ai_protocol)) == -1) {
            continue;
        }

        constexpr int yes = 1;
        if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            throw std::system_error(errno, std::system_category(), "Server::start setsockopt");
        }

        if (bind(m_socket, result->ai_addr, result->ai_addrlen) != 0) {
            close(m_socket);
            continue;
        }

        break;
    }

    if (p == nullptr) {
        throw std::system_error(errno, std::system_category(), "Server::start bind");
    }

    freeaddrinfo(result);

    if (listen(m_socket, backlog_size) != 0) {
        throw std::system_error(errno, std::system_category(), "Server::start listen");
    }

    if (exists(dump_path)) {
        std::ifstream file{dump_path};
        m_dictionary.load(file);
    }

    RequestHandler request_handler{m_dictionary};
    EventLoop event_loop{};
    auto acceptor = std::make_unique<Acceptor>(m_socket, event_loop, request_handler);
    event_loop.add_handler(m_socket, EPOLLIN, std::move(acceptor));
    event_loop.start();
}
