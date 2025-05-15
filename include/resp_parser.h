//
// Created by d4wgr on 4/23/2025.
//

#ifndef RESP_PARSER_H
#define RESP_PARSER_H

#include <optional>
#include <span>
#include <variant>
#include <vector>

#include "resp.h"

template<typename T>
inline constexpr bool always_false = false;

namespace resp {
    class Parser {
    public:
        explicit Parser(const size_t buffer_limit = 8192): m_buffer_limit{buffer_limit} {
        }

        void feed(std::span<const char> data);

        std::vector<Value> take_values();

    private:
        std::vector<Value> m_values{};
        std::vector<char> m_buffer{};
        size_t m_parsed{0};
        size_t m_position{0};
        size_t m_buffer_limit{};

        char advance() { return m_buffer[m_position++]; }

        [[nodiscard]] char peek() const { return m_buffer[m_position]; }

        std::optional<Value> decode_next();

        std::optional<SimpleString> decode_simple_string();

        std::optional<SimpleError> decode_simple_error();

        std::optional<Integer> decode_integer();

        std::optional<BulkString> decode_bulk_string();

        std::optional<Array> decode_array();
    };
}

#endif //RESP_PARSER_H
