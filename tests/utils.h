//
// Created by d4wgr on 5/20/2025.
//

#ifndef UTILS_H
#define UTILS_H
#include "resp.h"

resp::BulkString inline bulk_string(std::optional<std::string> str) {
    return resp::BulkString{std::move(str)};
}

template<typename... Args>
resp::Array make_array(Args &&... args) {
    std::vector<resp::Value> values;

    (values.emplace_back(resp::Value{
        []<typename T>(T &&arg) -> resp::Value {
            if constexpr (std::is_same_v<std::decay_t<T>, std::nullopt_t>) {
                return resp::BulkString{std::nullopt};
            } else {
                return resp::BulkString{std::make_optional(std::string(std::forward<T>(arg)))};
            }
        }(std::forward<Args>(args))
    }), ...);

    resp::Array array;
    array.value = std::move(values);
    return array;
}

#endif //UTILS_H
