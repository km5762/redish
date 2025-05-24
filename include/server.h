//
// Created by d4wgr on 5/3/2025.
//

#ifndef SERVER_H
#define SERVER_H

#include <condition_variable>
#include <queue>
#include <string_view>
#include <unistd.h>
#include <filesystem>

#include "connection.h"
#include "dictionary.h"

class Server {
public:
    [[noreturn]] void start(std::string_view port = "6379", int backlog_size = 128);

    ~Server() { close(m_socket); }

    inline static std::filesystem::path dump_path{"dump.dish"};

private:
    int m_socket{};
    std::unordered_map<int, Connection> m_connections{};
    Dictionary m_dictionary{};
};


#endif //SERVER_H
