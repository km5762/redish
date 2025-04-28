//
// Created by d4wgr on 4/23/2025.
//

#include "resp_codec.h"

#include <charconv>
#include <iostream>
#include <ostream>

std::optional<RespCodec::Message> RespCodec::decode(const std::string_view data) {
    m_data = data;
    std::optional<Message> message{};
    switch (advance()) {
        case '+':
            message = decode_simple_string();
            break;
        case '-':
            message = decode_simple_error();
            break;
        case ':':
            message = decode_integer();
            break;
        default:
            message = std::nullopt;
    }

    // if we have not consumed all the input
    if (m_position != data.size()) {
        return std::nullopt;
    }
    return message;
}

std::optional<RespCodec::SimpleString> RespCodec::decode_simple_string() {
    const size_t start{m_position};

    while (m_position + 1 < m_data.size()) {
        if (advance() == '\r' && peek() == '\n') {
            const size_t size{m_position - start - 1};
            advance(); // consume newline
            std::string value{m_data.data() + start, size};
            return SimpleString{std::move(value)};
        }
    }

    return std::nullopt;
}

std::optional<RespCodec::SimpleError> RespCodec::decode_simple_error() {
    const size_t prefix_start{m_position};
    SimpleError error{};
    bool prefix_parsed{false};

    while (m_position + 1 < m_data.size()) {
        const char c = advance();
        if (c == ' ') {
            const size_t size{m_position - prefix_start - 1};
            std::string prefix{m_data.data() + prefix_start, size};
            error.prefix = std::move(prefix);
            prefix_parsed = true;
            break;
        }
        if (c == '\r' && peek() == '\n') {
            const size_t size{m_position - prefix_start - 1};
            std::string prefix{m_data.data() + prefix_start, size};
            advance(); // consume newline
            error.prefix = std::move(prefix);
            return error;
        }
    }

    if (!prefix_parsed) {
        return std::nullopt;
    }

    const size_t value_start{m_position};
    while (m_position + 1 < m_data.size()) {
        if (advance() == '\r' && peek() == '\n') {
            const size_t size{m_position - value_start - 1};
            std::string value{m_data.data() + value_start, size};
            advance(); // consume newline
            error.value = std::move(value);
            return error;
        }
    }

    return std::nullopt;
}

std::optional<RespCodec::Integer> RespCodec::decode_integer() {
    size_t start{};
    bool start_set{false};

    while (m_position + 1 < m_data.size()) {
        const char c = advance();
        if (!start_set && c != '+') {
            start = m_position - 1;
            start_set = true;
        }
        if (c == '\r' && peek() == '\n') {
            const size_t size{m_position - start - 1};
            advance(); // consume newline
            const std::string_view value{m_data.data() + start, size};
            int int_value{};
            auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), int_value);

            if (ec != std::errc() || ptr != value.end()) {
                return std::nullopt;
            }
            return Integer{int_value};
        }
    }

    return std::nullopt;
}
