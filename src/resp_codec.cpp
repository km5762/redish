//
// Created by d4wgr on 4/23/2025.
//

#include "resp_codec.h"

#include <charconv>
#include <format>
#include <iostream>

std::optional<RespCodec::Message> RespCodec::decode(const std::string_view data) {
    m_data = data;

    std::optional<Message> message = decode_next();

    // if we have not consumed all the input
    if (m_position != m_data.size()) {
        return std::nullopt;
    }
    return message;
}

std::optional<RespCodec::Message> RespCodec::decode_next() {
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
        case '$':
            message = decode_bulk_string();
            break;
        case '*':
            message = decode_array();
            break;
        default:
            message = std::nullopt;
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

std::optional<RespCodec::BulkString> RespCodec::decode_bulk_string() {
    const size_t start{m_position};
    size_t size{};

    while (m_position + 1 < m_data.size()) {
        if (advance() == '\r' && peek() == '\n') {
            const std::string_view value{m_data.data() + start, m_position - start - 1};
            advance(); // consume newline

            if (value == "-1") {
                return BulkString{std::nullopt};
            }

            auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), size);

            if (ec != std::errc()) {
                return std::nullopt;
            }
            break;
        }
    }


    std::string value{m_data.data() + m_position, size};
    m_position += size;


    if (m_position >= m_data.size() || advance() != '\r' || peek() != '\n') {
        return std::nullopt;
    }

    advance(); // consume newline

    return BulkString{std::move(value)};
}

std::optional<RespCodec::Array> RespCodec::decode_array() {
    const size_t start{m_position};
    size_t size{};

    while (m_position + 1 < m_data.size()) {
        if (advance() == '\r' && peek() == '\n') {
            const std::string_view value{m_data.data() + start, m_position - start - 1};
            advance(); // consume newline

            if (value == "-1") {
                return Array{std::nullopt};
            }

            auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), size);

            if (ec != std::errc()) {
                return std::nullopt;
            }
            break;
        }
    }

    std::vector<Message> messages{size};
    for (size_t i = 0; i < size; ++i) {
        std::optional message = decode_next();

        if (!message.has_value()) {
            return std::nullopt;
        }

        messages[i] = std::move(*message);
    }

    return Array{std::move(messages)};
}

std::string RespCodec::encode(const Message &message) {
    return std::visit([this](auto &&msg) -> std::string {
        using T = std::decay_t<decltype(msg)>;

        if constexpr (std::is_same_v<T, SimpleString>) {
            return encode_simple_string(msg);
        } else if constexpr (std::is_same_v<T, SimpleError>) {
            return encode_simple_error(msg);
        } else if constexpr (std::is_same_v<T, Integer>) {
            return encode_integer(msg);
        } else if constexpr (std::is_same_v<T, BulkString>) {
            return encode_bulk_string(msg);
        } else if constexpr (std::is_same_v<T, Array>) {
            return encode_array(msg);
        } else {
            static_assert(always_false<T>, "Non-exhaustive visitor");
        }

        return {};
    }, message);
}

std::string RespCodec::encode_simple_string(const SimpleString &simple_string) {
    return std::format("+{}\r\n", simple_string.value);
}

std::string RespCodec::encode_simple_error(const SimpleError &simple_error) {
    return std::format("-{} {}\r\n", simple_error.prefix, simple_error.value);
}

std::string RespCodec::encode_integer(const Integer &integer) {
    return std::format(":{}\r\n", integer.value);
}

std::string RespCodec::encode_bulk_string(const BulkString &bulk_string) {
    if (!bulk_string.value.has_value()) {
        return "$-1\r\n";
    }
    return std::format("${}\r\n{}\r\n", bulk_string.value->size(), *bulk_string.value);
}

std::string RespCodec::encode_array(const Array &array) {
    if (!array.value.has_value()) {
        return "*-1\r\n";
    }

    std::string encoded{std::format("*{}\r\n", array.value->size())};

    for (const Message &message: *array.value) {
        encoded += encode(message);
    }

    return encoded;
}





