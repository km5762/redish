//
// Created by d4wgr on 4/23/2025.
//

#ifndef RESP_H
#define RESP_H

#include <optional>
#include <string>
#include <variant>
#include <vector>

template<typename T>
inline constexpr bool always_false = false;

class RespCodec {
public:
    struct SimpleString {
        std::string value;

        bool operator==(const SimpleString &) const = default;
    };

    struct SimpleError {
        std::string prefix;
        std::string value;

        bool operator==(const SimpleError &) const = default;
    };

    struct Integer {
        int value;

        bool operator==(const Integer &) const = default;
    };

    struct BulkString {
        std::optional<std::string> value;

        bool operator==(const BulkString &) const = default;
    };

    struct Array;
    using Message = std::variant<SimpleString, SimpleError, Integer, BulkString, Array>;

    struct Array {
        std::optional<std::vector<Message> > value;

        bool operator==(const Array &) const = default;
    };

    std::optional<Message> decode(std::string_view data);

    static std::string encode(const Message &message);

private:
    std::string_view m_data{};
    size_t m_position{0};

    char advance() { return m_data[m_position++]; }

    [[nodiscard]] char peek() const { return m_data[m_position]; }

    [[nodiscard]] bool at_data_end() const { return m_position >= m_data.size(); }

    std::optional<Message> decode_next();

    std::optional<SimpleString> decode_simple_string();

    std::optional<SimpleError> decode_simple_error();

    std::optional<Integer> decode_integer();

    std::optional<BulkString> decode_bulk_string();

    std::optional<Array> decode_array();

    static std::string encode_simple_string(const SimpleString &simple_string);

    static std::string encode_simple_error(const SimpleError &simple_error);

    static std::string encode_integer(const Integer &integer);

    static std::string encode_bulk_string(const BulkString &bulk_string);

    static std::string encode_array(const Array &array);
};


#endif //RESP_H
