//
// Created by d4wgr on 5/8/2025.
//

#include "requests.h"

#include <algorithm>
#include <cassert>
#include <format>

namespace {
    void handle_ping(const std::vector<resp::Message> &tokens, std::ostream &stream) {
        assert(!tokens.empty());

        resp::Message response;
        if (tokens.size() == 1) {
            encode(resp::SimpleString{"PONG"}, stream);
        } else {
            const auto argument = std::get_if<resp::BulkString>(&tokens[1]);
            if (argument == nullptr) {
                resp::SimpleError error{"ERR Protocol error: PING argument must be bulk string"};
                encode(error, stream);
                return;
            }
            encode(*argument, stream);
        }
    }

    void handle_set(const std::vector<resp::Message> &tokens, std::ostream &stream, Dictionary &dictionary) {
        if (tokens.size() < 3) {
            encode(resp::SimpleError{"ERR wrong number of arguments for 'set' command"}, stream);
            return;
        }

        const auto key = std::get_if<resp::BulkString>(&tokens[1]);
        if (key == nullptr || !key->value.has_value()) {
            encode(resp::SimpleError{"ERR key must be non-null bulk string"}, stream);
            return;
        }

        const auto value = std::get_if<resp::BulkString>(&tokens[2]);
        if (value == nullptr || !value->value.has_value()) {
            encode(resp::SimpleError{"ERR value must be non-null bulk string"}, stream);
            return;
        }

        dictionary.set(*key->value, *value);

        encode(resp::SimpleString{"OK"}, stream);
    }

    void handle_get(const std::vector<resp::Message> &tokens, std::ostream &stream, Dictionary &dictionary) {
        if (tokens.size() < 2) {
            return encode(resp::SimpleError{"ERR wrong number of arguments for 'get' command"}, stream);
        }

        const auto key = std::get_if<resp::BulkString>(&tokens[1]);
        if (!key || !key->value) {
            return encode(resp::SimpleError{"ERR key must be non-null bulk string"}, stream);
        }

        const auto value = dictionary.get(*key->value);
        if (!value) {
            return encode(resp::BulkString{std::nullopt}, stream);
        }

        if (!std::holds_alternative<resp::BulkString>((*value).get())) {
            return encode(resp::SimpleError{"ERR non bulk-string value"}, stream);
        }

        encode(*value, stream);
    }


    void handle_command(const resp::Array &command, std::ostream &stream, Dictionary &dictionary) {
        const auto &tokens = command.value;

        if (!tokens.has_value() || tokens->empty()) {
            resp::SimpleError error{"ERR Protocol error: invalid multibulk length"};
            encode(error, stream);
        }

        const auto first = std::get_if<resp::BulkString>(&tokens->front());
        if (first == nullptr) {
            resp::SimpleError error{"ERR Protocol error: missing command name"};
            encode(error, stream);
            return;
        }


        std::string name{*first->value};
        std::ranges::transform(name, name.begin(), [](const unsigned char c) {
            return std::toupper(c);
        });

        if (name == "PING") {
            handle_ping(*tokens, stream);
        } else if (name == "SET") {
            handle_set(*tokens, stream, dictionary);
        } else if (name == "GET") {
            handle_get(*tokens, stream, dictionary);
        } else {
            encode(resp::SimpleError{std::format("ERR unknown command '{}'", name)}, stream);
        }
    }
}


namespace requests {
    void handle(const resp::Message &request, std::ostream &stream, Dictionary &dictionary) {
        if (const auto command = std::get_if<resp::Array>(&request)) {
            handle_command(*command, stream, dictionary);
        }
    }
}
