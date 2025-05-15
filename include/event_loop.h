//
// Created by d4wgr on 5/14/2025.
//

#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

class EventLoop {
public:
    class Handler {
    public:
        virtual ~Handler() = default;

        virtual void handle(uint32_t events) = 0;
    };

    explicit EventLoop();

    void start();

    void add_handler(int fd, uint32_t events, std::unique_ptr<Handler> handler);

    void remove_handler(int fd);

    void modify_handler(int fd, uint32_t events) const;

private:
    static constexpr int max_events{1024};

    int m_epoll{-1};
    std::unordered_map<int, std::unique_ptr<Handler> > m_handlers{};
    std::vector<int> m_removed_fds{};
};


#endif //EVENT_LOOP_H
