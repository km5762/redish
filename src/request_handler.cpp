//
// Created by d4wgr on 5/12/2025.
//

#include "request_handler.h"
#include "resp.h"
#include "dictionary.h"

#include <cassert>
#include <format>
#include <fstream>
#include <algorithm>

void RequestHandler::handle(const resp::Value &request, Connection &connection) const {
    if (const auto command = std::get_if<resp::Array>(&request)) {
        handle_command(*command, connection);
    }
}

void RequestHandler::handle_command(const resp::Array &command, Connection &connection) const {
    const auto &tokens = command.value;

    if (!tokens.has_value() || tokens->empty()) {
        resp::SimpleError error{"ERR Protocol error: invalid multibulk length"};
        connection.send(error);
    }

    const auto first = std::get_if<resp::BulkString>(&tokens->front());
    if (first == nullptr) {
        resp::SimpleError error{"ERR Protocol error: missing command name"};
        connection.send(error);
        return;
    }


    std::string name{*first->value};
    std::ranges::transform(name, name.begin(), [](const unsigned char c) {
        return std::toupper(c);
    });

    if (name == "PING") {
        handle_ping(*tokens, connection);
    } else if (name == "SET") {
        handle_set(*tokens, connection);
    } else if (name == "GET") {
        handle_get(*tokens, connection);
    } else if (name == "FLUSHDB") {
        handle_flushdb(*tokens, connection);
    } else {
        connection.send(resp::SimpleError{std::format("ERR unknown command '{}'", name)});
    }
}

void RequestHandler::handle_ping(const std::vector<resp::Value> &tokens, Connection &connection) {
    assert(!tokens.empty());

    resp::Value response;
    if (tokens.size() == 1) {
        connection.send(resp::SimpleString{"PONG"});
    } else {
        const auto argument = std::get_if<resp::BulkString>(&tokens[1]);
        if (argument == nullptr) {
            connection.send(resp::SimpleError{"ERR Protocol error: PING argument must be bulk string"});
            return;
        }
        connection.send(*argument);
    }
}

void RequestHandler::handle_set(const std::vector<resp::Value> &tokens, Connection &connection) const {
    if (tokens.size() < 3) {
        connection.send(resp::SimpleError{"ERR wrong number of arguments for 'set' command"});
        return;
    }

    const auto key = std::get_if<resp::BulkString>(&tokens[1]);
    if (key == nullptr || !key->value.has_value()) {
        connection.send(resp::SimpleError{"ERR key must be non-null bulk string"});
        return;
    }

    const auto value = std::get_if<resp::BulkString>(&tokens[2]);
    if (value == nullptr || !value->value.has_value()) {
        connection.send(resp::SimpleError{"ERR value must be non-null bulk string"});
        return;
    }

    m_dictionary.set(*key->value, *value);

    connection.send(resp::SimpleString{"OK"});
}

void RequestHandler::handle_get(const std::vector<resp::Value> &tokens, Connection &connection) const {
    if (tokens.size() < 2) {
        return connection.send(resp::SimpleError{"ERR wrong number of arguments for 'get' command"});
    }

    const auto key = std::get_if<resp::BulkString>(&tokens[1]);
    if (!key || !key->value) {
        return connection.send(resp::SimpleError{"ERR key must be non-null bulk string"});
    }

    const auto value = m_dictionary.get(*key->value);
    if (!value) {
        return connection.send(resp::BulkString{std::nullopt});
    }

    if (!std::holds_alternative<resp::BulkString>(value->get())) {
        return connection.send(resp::SimpleError{"ERR non bulk-string value"});
    }

    connection.send(*value);
}

void RequestHandler::handle_flushdb(const std::vector<resp::Value> &tokens, Connection &connection) const {
    m_dictionary.flush();
    connection.send(resp::SimpleString{"OK"});
}


