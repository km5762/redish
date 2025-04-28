//
// Created by d4wgr on 4/23/2025.
//

#ifndef RESP_H
#define RESP_H

#include <optional>
#include <string>
#include <variant>
#include <vector>

class RespCodec {
public:
    struct SimpleString {
        std::string value{};
    };

    struct SimpleError {
        std::string prefix{};
        std::string value{};
    };

    struct Integer {
        int value{};
    };

    struct BulkString {
        std::optional<std::string> value{};
    };

    struct Array;
    using Message = std::variant<SimpleString, SimpleError, Integer, BulkString, Array>;

    struct Array {
        std::vector<Message> value{};
    };

    std::optional<Message> decode(std::string_view data);

private:
    std::string_view m_data{};
    size_t m_position{0};

    char advance() { return m_data[m_position++]; }

    char peek() const { return m_data[m_position]; }

    bool at_data_end() const { return m_position >= m_data.size(); };

    std::optional<SimpleString> decode_simple_string();

    std::optional<SimpleError> decode_simple_error();

    std::optional<Integer> decode_integer();
};


#endif //RESP_H
