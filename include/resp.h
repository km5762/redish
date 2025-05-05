//
// Created by d4wgr on 4/23/2025.
//

#ifndef RESP_H
#define RESP_H

#include <istream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

template<typename T>
inline constexpr bool always_false = false;

namespace resp {
    struct SimpleString {
        std::string value;

        bool operator==(const SimpleString &) const = default;
    };

    struct SimpleError {
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

    std::optional<Message> decode(std::istream &stream);

    std::string encode(const Message &message);
};


#endif //RESP_H
