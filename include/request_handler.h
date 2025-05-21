//
// Created by d4wgr on 5/12/2025.
//

#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "connection.h"
#include "dictionary.h"
#include "resp.h"
#include "tokenizer.h"

class Connection;

class RequestHandler {
public:
    explicit RequestHandler(Dictionary &dictionary): m_dictionary{dictionary} {
    }

    void handle(const resp::Value &request, Connection &connection) const;

private:
    Dictionary &m_dictionary;

    void handle_command(const resp::Array &command, Connection &connection) const;

    static void handle_ping(const Tokenizer &tokens, Connection &connection);

    void handle_set(const Tokenizer &tokens, Connection &connection) const;

    void handle_get(const Tokenizer &tokens, Connection &connection) const;

    void handle_flushdb(const Tokenizer &tokens, Connection &connection) const;

    void handle_exists(const Tokenizer &tokens, Connection &connection) const;

    void handle_del(const Tokenizer &tokens, Connection &connection) const;

    void handle_incr(const Tokenizer &tokens, Connection &connection) const;

    void handle_decr(const Tokenizer &tokens, Connection &connection) const;
};


#endif //REQUEST_HANDLER_H
