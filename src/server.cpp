//
// Created by d4wgr on 5/3/2025.
//

#include "server.h"

#include <array>
#include <cassert>
#include <format>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdexcept>
#include <system_error>
#include <iostream>

#include "resp.h"
#include "tcp_streambuf.h"
#include "thread_pool.h"

namespace {
    void handle_ping(const std::vector<resp::Message> &tokens, std::ostream &stream) {
        assert(!tokens.empty());

        resp::Message response;
        if (tokens.size() == 1) {
            encode(resp::SimpleString{"PONG"}, stream);
        } else {
            const auto argument = std::get_if<resp::BulkString>(&tokens[1]);
            if (argument == nullptr) {
                resp::SimpleError error{"ERR Protocol error: malformed PING argument"};
                encode(error, stream);
            }
            encode(*argument, stream);
        }
    }

    void handle_command(const resp::Array &command, std::ostream &stream) {
        const auto &tokens = command.value;

        if (!tokens.has_value() || tokens->empty()) {
            resp::SimpleError error{"ERR Protocol error: invalid multibulk length"};
            encode(error, stream);
        }

        const auto first = std::get_if<resp::BulkString>(&tokens->front());
        if (first == nullptr) {
            resp::SimpleError error{"ERR Protocol error: missing command name"};
            encode(error, stream);
        }


        const auto name = *first->value;
        if (name == "PING") {
            handle_ping(*tokens, stream);
        } else {
            encode(resp::SimpleString{std::format("ERR unknown command '{}'", name)}, stream);
        }
    }

    void handle_client(const int socket) {
        TcpStreambuf<1> tcp_streambuf{socket};
        std::iostream stream{&tcp_streambuf};

        while (auto message = resp::decode(stream)) {
            if (const auto command = std::get_if<resp::Array>(&*message)) {
                handle_command(*command, stream);
            } else {
                return;
            }
        }

        close(socket);
    }
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

    freeaddrinfo(result);

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
            throw std::system_error(errno, std::system_category(), "Server::start accept");
        }

        thread_pool.enqueue([client_socket] {
            handle_client(client_socket);
        });
    }
}
