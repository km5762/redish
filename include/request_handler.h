//
// Created by d4wgr on 5/12/2025.
//

#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "connection.h"
#include "dictionary.h"
#include "resp.h"

class Connection;

class RequestHandler {
public:
    explicit RequestHandler(Dictionary &dictionary): m_dictionary{dictionary} {
    }

    void handle(const resp::Value &request, Connection &connection) const;

private:
    Dictionary &m_dictionary;

    void handle_command(const resp::Array &command, Connection &connection) const;

    static void handle_ping(const std::vector<resp::Value> &tokens, Connection &connection);

    void handle_set(const std::vector<resp::Value> &tokens, Connection &connection) const;

    void handle_get(const std::vector<resp::Value> &tokens, Connection &connection) const;

    void handle_flushdb(const std::vector<resp::Value> &tokens, Connection &connection) const;
};


#endif //REQUEST_HANDLER_H
