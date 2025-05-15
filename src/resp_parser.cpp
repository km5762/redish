//
// Created by d4wgr on 4/23/2025.
//

#include "resp_parser.h"
#include "resp.h"

#include <charconv>
#include <format>
#include <iostream>

namespace resp {
    void Parser::feed(const std::span<const char> data) {
        m_buffer.insert(m_buffer.end(), data.begin(), data.end());

        std::optional<Value> value = decode_next();

        if (value.has_value()) {
            m_values.emplace_back(std::move(*value));
            // if we get a successful parse, increment the parsed offset to the new position
            m_parsed = m_position;

            // if we have expanded over an internal limit, compact the buffer by erasing the parsed portion
            if (m_parsed > m_buffer_limit) {
                m_buffer.erase(m_buffer.begin(),
                               m_buffer.begin() + static_cast<std::vector<char>::difference_type>(m_parsed));
                m_parsed = 0;
                m_position = 0;
            }
        } else {
            // otherwise, reset the position back to the last successful parse
            m_position = m_parsed;
        }
    }

    std::vector<Value> Parser::take_values() {
        std::vector<Value> values = std::move(m_values);
        m_values.clear();
        return values;
    }

    std::optional<Value> Parser::decode_next() {
        std::optional<Value> value{};
        switch (advance()) {
            case '+':
                value = decode_simple_string();
                break;
            case '-':
                value = decode_simple_error();
                break;
            case ':':
                value = decode_integer();
                break;
            case '$':
                value = decode_bulk_string();
                break;
            case '*':
                value = decode_array();
                break;
            default:
                value = std::nullopt;
        }

        return value;
    }

    std::optional<SimpleString> Parser::decode_simple_string() {
        const size_t start{m_position};

        while (m_position + 1 < m_buffer.size()) {
            if (advance() == '\r' && peek() == '\n') {
                const size_t size{m_position - start - 1};
                advance(); // consume newline
                std::string value{m_buffer.data() + start, size};
                return SimpleString{std::move(value)};
            }
        }

        return std::nullopt;
    }

    std::optional<SimpleError> Parser::decode_simple_error() {
        const size_t prefix_start{m_position};
        SimpleError error{};
        bool prefix_parsed{false};

        while (m_position + 1 < m_buffer.size()) {
            const char c = advance();
            if (c == ' ') {
                const size_t size{m_position - prefix_start - 1};
                std::string prefix{m_buffer.data() + prefix_start, size};
                error.prefix = std::move(prefix);
                prefix_parsed = true;
                break;
            }
            if (c == '\r' && peek() == '\n') {
                const size_t size{m_position - prefix_start - 1};
                std::string prefix{m_buffer.data() + prefix_start, size};
                advance(); // consume newline
                error.prefix = std::move(prefix);
                return error;
            }
        }

        if (!prefix_parsed) {
            return std::nullopt;
        }

        const size_t value_start{m_position};
        while (m_position + 1 < m_buffer.size()) {
            if (advance() == '\r' && peek() == '\n') {
                const size_t size{m_position - value_start - 1};
                std::string value{m_buffer.data() + value_start, size};
                advance(); // consume newline
                error.value = std::move(value);
                return error;
            }
        }

        return std::nullopt;
    }

    std::optional<Integer> Parser::decode_integer() {
        size_t start{};
        bool start_set{false};

        while (m_position + 1 < m_buffer.size()) {
            const char c = advance();
            if (!start_set && c != '+') {
                start = m_position - 1;
                start_set = true;
            }
            if (c == '\r' && peek() == '\n') {
                const size_t size{m_position - start - 1};
                advance(); // consume newline
                const std::string_view value{m_buffer.data() + start, size};
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

    std::optional<BulkString> Parser::decode_bulk_string() {
        const size_t start{m_position};
        size_t size{};

        while (m_position + 1 < m_buffer.size()) {
            if (advance() == '\r' && peek() == '\n') {
                const std::string_view value{m_buffer.data() + start, m_position - start - 1};
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


        std::string value{m_buffer.data() + m_position, size};
        m_position += size;


        if (m_position >= m_buffer.size() || advance() != '\r' || peek() != '\n') {
            return std::nullopt;
        }

        advance(); // consume newline

        return BulkString{std::move(value)};
    }

    std::optional<Array> Parser::decode_array() {
        const size_t start{m_position};
        size_t size{};

        while (m_position + 1 < m_buffer.size()) {
            if (advance() == '\r' && peek() == '\n') {
                const std::string_view value{m_buffer.data() + start, m_position - start - 1};
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

        std::vector<Value> values{size};
        for (size_t i = 0; i < size; ++i) {
            std::optional value = decode_next();

            if (!value.has_value()) {
                return std::nullopt;
            }

            values[i] = std::move(*value);
        }

        return Array{std::move(values)};
    }
}
