//
// Created by d4wgr on 5/12/2025.
//

#ifndef RESP_H
#define RESP_H

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace resp {
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
    using Value = std::variant<SimpleString, SimpleError, Integer, BulkString, Array>;

    struct Array {
        std::optional<std::vector<Value> > value;

        bool operator==(const Array &) const = default;
    };

    inline constexpr Value nil{BulkString{std::nullopt}};
    inline constexpr Value ok{SimpleString{"OK"}};
    inline constexpr Value syntax_error{SimpleError{"ERR", "syntax error"}};

    void serialize(const Value &value, std::vector<char> &out);
}

#endif //RESP_H
