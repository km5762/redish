//
// Created by d4wgr on 5/14/2025.
//

#include "event_loop.h"

#include <fcntl.h>
#include <format>
#include <iostream>
#include <system_error>
#include <sys/epoll.h>

EventLoop::EventLoop() {
    m_epoll = epoll_create1(EPOLL_CLOEXEC);
    if (m_epoll == -1) {
        throw std::system_error(errno, std::system_category(), "Server::start epoll_create");
    }
}

void EventLoop::start() {
    while (true) {
        epoll_event events[max_events];
        for (const int fd: m_removed_fds) {
            m_handlers.erase(fd);
        }
        m_removed_fds.clear();
        const int number_of_events = epoll_wait(m_epoll, events, max_events, -1);

        if (number_of_events == -1) {
            throw std::system_error(errno, std::system_category(), "EventLoop::start epoll_wait");
        }

        for (size_t i{0}; i < number_of_events; ++i) {
            const int fd = events[i].data.fd;

            if (!m_handlers.contains(fd)) { continue; }

            m_handlers.at(fd)->handle(events[i].events);
        }
    }
}

void EventLoop::add_handler(const int fd, const uint32_t events, std::unique_ptr<Handler> handler) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        throw std::system_error(errno, std::system_category(), "EventLoop::add_handler fcntl");
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        throw std::system_error(errno, std::system_category(), "EventLoop::add_handler fcntl");
    }

    epoll_event event{};
    event.events = events;
    event.data.fd = fd;
    if (epoll_ctl(m_epoll, EPOLL_CTL_ADD, fd, &event) == -1) {
        throw std::system_error(errno, std::system_category(), "EventLoop::add_handler epoll_ctl");
    }

    m_handlers[fd] = std::move(handler);
}

void EventLoop::remove_handler(const int fd) {
    if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        throw std::system_error(errno, std::system_category(), "EventLoop::remove_handler epoll_ctl");
    }

    m_removed_fds.push_back(fd);
}

void EventLoop::modify_handler(const int fd, const uint32_t events) const {
    epoll_event event{};
    event.events = events;
    event.data.fd = fd;
    if (epoll_ctl(m_epoll, EPOLL_CTL_MOD, fd, &event) == -1) {
        throw std::system_error(errno, std::system_category(), "EventLoop::modify_handler epoll_ctl");
    }
}
