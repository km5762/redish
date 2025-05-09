//
// Created by d4wgr on 5/3/2025.
//

#ifndef SERVER_H
#define SERVER_H

#include <condition_variable>
#include <queue>
#include <string_view>
#include <unistd.h>

#include "dictionary.h"

class Server {
public:
    [[noreturn]] void start(std::string_view port = DEFAULT_PORT, int backlog_size = DEFAULT_BACKLOG_SIZE);

    ~Server() { close(m_socket); }

private:
    static constexpr int DEFAULT_BACKLOG_SIZE{128};
    static constexpr std::string_view DEFAULT_PORT{"6379"};

    int m_socket{};
    Dictionary m_dictionary{};

    void handle_ping(const std::vector<resp::Message> &tokens, std::ostream &stream);

    void handle_set(const std::vector<resp::Message> &tokens, std::ostream &stream);

    void handle_get(const std::vector<resp::Message> &tokens, std::ostream &stream);

    void handle_command(const resp::Array &command, std::ostream &stream);

    void handle_client(int socket);
};


#endif //SERVER_H
