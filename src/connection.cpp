//
// Created by d4wgr on 5/10/2025.
//

#include "connection.h"

#include <fcntl.h>
#include <format>
#include <iostream>
#include <system_error>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include "request_handler.h"

void Connection::handle(const uint32_t events) {
    if (events & EPOLLIN) {
        handle_receive();
    } else if (events & EPOLLOUT) {
        handle_send();
    }
}

void Connection::handle_receive() {
    const ssize_t bytes_received = recv(m_socket, m_read_buffer.data(), buffer_size, 0);
    if (bytes_received == -1) {
        // if recv fails for normal reason (read not actually available)
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        throw std::system_error(errno, std::system_category(), "Connection::Connection recv");
    }

    // either the connection was closed gracefully by the client, or the connection is broken in some way
    if (bytes_received <= 0) {
        m_event_loop.remove_handler(m_socket);
        return;
    }

    m_parser.feed(std::span{m_read_buffer.data(), bytes_received});

    for (const resp::Value &value: m_parser.take_values()) {
        m_request_handler.handle(value, *this);
    }
}

void Connection::handle_send() {
    while (m_bytes_remaining > 0) {
        const ssize_t bytes_sent = ::send(m_socket, m_write_buffer.data() + m_bytes_sent, m_bytes_remaining, 0);

        // write not actually available
        if (bytes_sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return;
        }

        // client has closed the connection, or connection is no longer good
        if (bytes_sent == -1) {
            m_event_loop.remove_handler(m_socket);
            return;
        }

        m_bytes_sent += bytes_sent;
        m_bytes_remaining -= bytes_sent;
    }

    m_write_buffer.clear();
    m_bytes_sent = 0;
    m_event_loop.modify_handler(m_socket, EPOLLIN);
}

void Connection::send(const resp::Value &value) {
    const size_t before = m_write_buffer.size();
    serialize(value, m_write_buffer);
    // subscribe to write events
    m_event_loop.modify_handler(m_socket, EPOLLIN | EPOLLOUT);
    m_bytes_remaining += m_write_buffer.size() - before;
}
