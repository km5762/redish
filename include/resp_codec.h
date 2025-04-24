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
        std::string value;
    };

    struct SimpleError {
        std::string value;
    };

    struct Integer {
        int value;
    };

    struct BulkString {
        std::optional<std::string> value;
    };

    struct Array;
    using Message = std::variant<SimpleString, SimpleError, Integer, BulkString, Array>;

    struct Array {
        std::vector<Message> value;
    };

    std::optional<Message> deserialize(std::string_view data);


};


#endif //RESP_H
