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

namespace {
    bool ichar_equals(const char a, const char b) {
        return std::tolower(static_cast<unsigned char>(a)) ==
               std::tolower(static_cast<unsigned char>(b));
    }

    bool iequals(std::string_view lhs, std::string_view rhs) {
        return std::ranges::equal(lhs, rhs, ichar_equals);
    }
}

void RequestHandler::handle(const resp::Value &request, Connection &connection) const {
    if (const auto command = std::get_if<resp::Array>(&request)) {
        handle_command(*command, connection);
    }
}

void RequestHandler::handle_command(const resp::Array &command, Connection &connection) const {
    const auto &tokens = command.value;

    if (!tokens.has_value() || tokens->empty()) {
        connection.send(resp::syntax_error);
        return;
    }

    const auto first = std::get_if<resp::BulkString>(&tokens->front());
    if (first == nullptr) {
        connection.send(resp::syntax_error);
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
            connection.send(resp::syntax_error);
            return;
        }
        connection.send(*argument);
    }
}

void RequestHandler::handle_set(const std::vector<resp::Value> &tokens, Connection &connection) const {
    if (tokens.size() < 3) {
        connection.send(resp::syntax_error);
        return;
    }

    const auto key = std::get_if<resp::BulkString>(&tokens[1]);
    if (key == nullptr || !key->value.has_value()) {
        connection.send(resp::syntax_error);
        return;
    }

    const auto value = std::get_if<resp::BulkString>(&tokens[2]);
    if (value == nullptr || !value->value.has_value()) {
        connection.send(resp::syntax_error);
        return;
    }


    bool nx{false};
    bool xx{false};
    bool get{false};

    for (size_t i{3}; i < tokens.size(); ++i) {
        const auto option = std::get_if<resp::BulkString>(&tokens[i]);

        if (option == nullptr || !option->value.has_value()) {
            connection.send(resp::syntax_error);
            return;
        }

        if (iequals(*option->value, "NX")) {
            if (xx) {
                connection.send(resp::syntax_error);
                return;
            }
            nx = true;
        } else if (iequals(*option->value, "XX")) {
            if (nx) {
                connection.send(resp::syntax_error);
                return;
            }
            xx = true;
        } else if (iequals(*option->value, "GET")) {
            get = true;
        } else {
            connection.send(resp::syntax_error);
            return;
        }
    }

    const bool contains = m_dictionary.contains(*key->value);
    if (nx && contains || xx && !contains) {
        connection.send(resp::nil);
        return;
    }

    if (get) {
        const auto response = m_dictionary.set_and_get(*key->value, *value);

        if (response.has_value() && std::holds_alternative<resp::BulkString>(*response)) {
            connection.send(*response);
            return;
        }
        connection.send(resp::nil);
        return;
    }

    m_dictionary.set(*key->value, *value);
    connection.send(resp::ok);
}

void RequestHandler::handle_get(const std::vector<resp::Value> &tokens, Connection &connection) const {
    if (tokens.size() < 2) {
        connection.send(resp::syntax_error);
        return;
    }

    const auto key = std::get_if<resp::BulkString>(&tokens[1]);
    if (!key || !key->value) {
        connection.send(resp::syntax_error);
        return;
    }

    const auto value = m_dictionary.get(*key->value);
    if (!value) {
        connection.send(resp::nil);
        return;
    }

    if (!std::holds_alternative<resp::BulkString>(value->get())) {
        connection.send(resp::syntax_error);
        return;
    }

    connection.send(*value);
}

void RequestHandler::handle_flushdb(const std::vector<resp::Value> &tokens, Connection &connection) const {
    m_dictionary.flush();
    connection.send(resp::ok);
}


