//
// Created by d4wgr on 5/12/2025.
//

#include "request_handler.h"
#include "resp.h"
#include "dictionary.h"

#include <cassert>
#include <format>

#include "tokenizer.h"

namespace {
    bool ichar_equals(const char a, const char b) {
        return std::tolower(static_cast<unsigned char>(a)) ==
               std::tolower(static_cast<unsigned char>(b));
    }

    bool iequals(std::string_view lhs, std::string_view rhs) {
        return std::ranges::equal(lhs, rhs, ichar_equals);
    }

    template<typename T>
    std::optional<T> try_parse_numeric(const std::string_view string) {
        T result;
        auto [ptr, ec] = std::from_chars(string.data(), string.data() + string.size(), result);

        if (ec == std::errc() && ptr == string.data() + string.size()) {
            return result;
        }
        return std::nullopt;
    }

    std::optional<int64_t> try_parse_positive_int(const std::string_view string) {
        std::optional result = try_parse_numeric<int64_t>(string);

        if (result.has_value() && *result > 0) {
            return result;
        }

        return std::nullopt;
    }

    const resp::SimpleError &get_incr_error_message(const Dictionary::incr_error e) {
        static const std::unordered_map<Dictionary::incr_error, resp::SimpleError> table = {
            {
                Dictionary::incr_error::non_bulk_string_value,
                {"ERR", "cannot increment or decrement a non-string value"}
            },
            {
                Dictionary::incr_error::null_bulk_string_value,
                {"ERR", "value is nil"}
            },
            {
                Dictionary::incr_error::non_numeric_value,
                {"ERR", "value is not an integer or out of range"}
            },
            {static_cast<Dictionary::incr_error>(-1), {"ERR", "unknown error"}}
        };

        if (table.contains(e)) {
            return table.at(e);
        }
        return table.at(static_cast<Dictionary::incr_error>(-1));
    }
}

void RequestHandler::handle(const resp::Value &request, Connection &connection) const {
    if (const auto command = std::get_if<resp::Array>(&request)) {
        handle_command(*command, connection);
    }
}

void RequestHandler::handle_command(const resp::Array &command, Connection &connection) const {
    if (!command.value.has_value()) {
        connection.send(resp::syntax_error);
        return;
    }

    const auto tokens = Tokenizer{*command.value};

    for (const auto &token: tokens) {
        if (!token.has_value()) {
            connection.send(resp::syntax_error);
            return;
        }
    }

    const auto name = **tokens.begin();

    if (iequals(name, "PING")) {
        handle_ping(tokens, connection);
    } else if (iequals(name, "SET")) {
        handle_set(tokens, connection);
    } else if (iequals(name, "GET")) {
        handle_get(tokens, connection);
    } else if (iequals(name, "FLUSHDB")) {
        handle_flushdb(tokens, connection);
    } else if (iequals(name, "EXISTS")) {
        handle_exists(tokens, connection);
    } else if (iequals(name, "DEL")) {
        handle_del(tokens, connection);
    } else if (iequals(name, "INCR")) {
        handle_incr(tokens, connection);
    } else if (iequals(name, "DECR")) {
        handle_decr(tokens, connection);
    } else if (iequals(name, "LPUSH")) {
        handle_lpush(tokens, connection);
    } else if (iequals(name, "RPUSH")) {
        handle_rpush(tokens, connection);
    } else if (iequals(name, "LRANGE")) {
        handle_lrange(tokens, connection);
    } else {
        connection.send(resp::SimpleError{std::format("ERR unknown command '{}'", name)});
    }
}

void RequestHandler::handle_ping(const Tokenizer &tokens, Connection &connection) {
    if (tokens.size() == 1) {
        connection.send(resp::SimpleString{"PONG"});
    } else if (tokens.size() == 2) {
        const resp::Value &argument = tokens.get_value(1);
        connection.send(argument);
    } else {
        connection.send(resp::syntax_error);
    }
}

void RequestHandler::handle_set(const Tokenizer &tokens, Connection &connection) const {
    if (tokens.size() < 3) {
        connection.send(resp::syntax_error);
        return;
    }

    const auto key = tokens.get_string(1);

    bool nx{false};
    bool xx{false};
    bool get{false};
    std::optional<Timestamp> expiry{std::nullopt};
    for (size_t i{3}; i < tokens.size(); ++i) {
        const auto option = *tokens.get_string(i);

        if (iequals(option, "NX")) {
            if (xx) {
                connection.send(resp::syntax_error);
                return;
            }
            nx = true;
        } else if (iequals(option, "XX")) {
            if (nx) {
                connection.send(resp::syntax_error);
                return;
            }
            xx = true;
        } else if (iequals(option, "GET")) {
            get = true;
        } else if (iequals(option, "EX")) {
            if (expiry.has_value() || i == tokens.size() - 1) {
                connection.send(resp::syntax_error);
                return;
            }

            const auto duration = try_parse_positive_int(*tokens.get_string(++i));
            if (!duration.has_value()) {
                connection.send(resp::syntax_error);
                return;
            }
            expiry = Clock::now() + std::chrono::seconds(*duration);
        } else if (iequals(option, "PX")) {
            if (expiry.has_value() || i == tokens.size() - 1) {
                connection.send(resp::syntax_error);
                return;
            }

            const auto duration = try_parse_positive_int(*tokens.get_string(++i));
            if (!duration.has_value()) {
                connection.send(resp::syntax_error);
                return;
            }
            expiry = Clock::now() + std::chrono::milliseconds(*duration);
        } else if (iequals(option, "EXAT")) {
            if (expiry.has_value() || i == tokens.size() - 1) {
                connection.send(resp::syntax_error);
                return;
            }

            const auto timestamp = try_parse_positive_int(*tokens.get_string(++i));
            if (!timestamp.has_value()) {
                connection.send(resp::syntax_error);
                return;
            }
            expiry = Timestamp{std::chrono::seconds(*timestamp)};
        } else if (iequals(option, "PXAT")) {
            if (expiry.has_value() || i == tokens.size() - 1) {
                connection.send(resp::syntax_error);
                return;
            }

            const auto timestamp = try_parse_positive_int(*tokens.get_string(++i));
            if (!timestamp.has_value()) {
                connection.send(resp::syntax_error);
                return;
            }
            expiry = Timestamp{std::chrono::milliseconds(*timestamp)};
        } else {
            connection.send(resp::syntax_error);
            return;
        }
    }

    const bool exists = m_dictionary.exists(key->data());
    if (nx && exists || xx && !exists) {
        connection.send(resp::nil);
        return;
    }

    const auto &value = tokens.get_value(2);
    if (get) {
        const auto response = m_dictionary.set_and_get(key->data(), value, expiry);

        if (response.has_value() && std::holds_alternative<resp::BulkString>(*response)) {
            connection.send(*response);
            return;
        }
        connection.send(resp::nil);
        return;
    }

    m_dictionary.set(key->data(), value, expiry);
    connection.send(resp::ok);
}

void RequestHandler::handle_get(const Tokenizer &tokens, Connection &connection) const {
    if (tokens.size() < 2) {
        connection.send(resp::syntax_error);
        return;
    }

    const auto key = tokens.get_string(1);

    const auto value = m_dictionary.get(key->data());
    if (!value) {
        connection.send(resp::nil);
        return;
    }

    if (!std::holds_alternative<resp::BulkString>(value->get())) {
        connection.send(resp::SimpleError{"ERR", "cannot get non-string type"});
        return;
    }

    connection.send(*value);
}

void RequestHandler::handle_flushdb(const Tokenizer &tokens, Connection &connection) const {
    if (tokens.size() > 2) {
        connection.send(resp::syntax_error);
        return;
    }
    m_dictionary.flush();
    connection.send(resp::ok);
}

void RequestHandler::handle_exists(const Tokenizer &tokens, Connection &connection) const {
    if (tokens.size() < 2) {
        connection.send(resp::syntax_error);
        return;
    }

    int count{0};
    for (size_t i{1}; i < tokens.size(); ++i) {
        const auto key = tokens.get_string(i);

        if (m_dictionary.exists(key->data())) {
            ++count;
        }
    }

    connection.send(resp::Integer{count});
}

void RequestHandler::handle_del(const Tokenizer &tokens, Connection &connection) const {
    if (tokens.size() < 2) {
        connection.send(resp::syntax_error);
        return;
    }

    int count{0};
    for (size_t i{1}; i < tokens.size(); ++i) {
        const auto key = tokens.get_string(i);

        if (m_dictionary.exists(key->data())) {
            m_dictionary.del(key->data());
            ++count;
        }
    }

    connection.send(resp::Integer{count});
}

void RequestHandler::handle_incr(const Tokenizer &tokens, Connection &connection) const {
    if (tokens.size() != 2) {
        connection.send(resp::syntax_error);
        return;
    }

    const auto key = tokens.get_string(1);

    const auto result = m_dictionary.incr(key->data());

    if (!result.has_value()) {
        connection.send(get_incr_error_message(result.error()));
        return;
    }

    connection.send(resp::Integer{result.value()});
}

void RequestHandler::handle_decr(const Tokenizer &tokens, Connection &connection) const {
    if (tokens.size() != 2) {
        connection.send(resp::syntax_error);
        return;
    }

    const auto key = tokens.get_string(1);

    const auto result = m_dictionary.incr(key->data(), -1);

    if (!result.has_value()) {
        connection.send(get_incr_error_message(result.error()));
        return;
    }

    connection.send(resp::Integer{result.value()});
}

void RequestHandler::handle_lpush(const Tokenizer &tokens, Connection &connection) const {
    if (tokens.size() < 2) {
        connection.send(resp::syntax_error);
        return;
    }

    const auto key = tokens.get_string(1);
    const resp::Value *ptr = &tokens.get_value(2);
    const std::span values{ptr, tokens.size() - 2};
    constexpr bool reverse = true;

    const ssize_t new_size = m_dictionary.push(key->data(), values, reverse);
    if (new_size == -1) {
        connection.send(resp::SimpleError{"ERR", "cannot push to non-array value"});
        return;
    }

    connection.send(resp::Integer{new_size});
}

void RequestHandler::handle_rpush(const Tokenizer &tokens, Connection &connection) const {
    if (tokens.size() < 2) {
        connection.send(resp::syntax_error);
        return;
    }

    const auto key = tokens.get_string(1);
    const resp::Value *ptr = &tokens.get_value(2);
    const std::span values{ptr, tokens.size() - 2};
    constexpr bool reverse = false;

    const ssize_t new_size = m_dictionary.push(key->data(), values, reverse);
    if (new_size == -1) {
        connection.send(resp::SimpleError{"ERR", "cannot push to non-array value"});
        return;
    }

    connection.send(resp::Integer{new_size});
}

void RequestHandler::handle_lrange(const Tokenizer &tokens, Connection &connection) const {
    if (tokens.size() != 4) {
        connection.send(resp::syntax_error);
        return;
    }

    const auto key = tokens.get_string(1);
    const auto start = try_parse_numeric<ptrdiff_t>(*tokens.get_string(2));
    const auto stop = try_parse_numeric<ptrdiff_t>(*tokens.get_string(3));

    if (!start || !stop) {
        connection.send(resp::syntax_error);
        return;
    }

    const auto range = m_dictionary.range(key->data(), *start, *stop);

    if (!range.has_value()) {
        connection.send(resp::SimpleError{"ERR", "cannot get range of non-array type"});
        return;
    }

    connection.send(resp::Array{
        std::vector<resp::Value>{range->begin(), range->end()}
    });
}




