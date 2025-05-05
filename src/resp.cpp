//
// Created by d4wgr on 4/23/2025.
//

#include "resp.h"

#include <charconv>
#include <format>
#include <iostream>

namespace {
    std::optional<std::string> decode_string(std::istream &stream, int length = -1) {
        std::string value{};
        if (length > 0) {
            value.reserve(length);
        }
        if (std::getline(stream, value, '\r')) {
            if (stream.peek() == '\n') {
                stream.ignore(); // consume '\n'
                return std::move(value);
            }
        }
        return std::nullopt;
    }

    std::optional<resp::SimpleString> decode_simple_string(std::istream &stream) {
        if (auto value = decode_string(stream)) {
            return resp::SimpleString{std::move(*value)};
        }

        return std::nullopt;
    }

    std::optional<resp::SimpleError> decode_simple_error(std::istream &stream) {
        if (auto value = decode_string(stream)) {
            return resp::SimpleError{std::move(*value)};
        }

        return std::nullopt;
    }

    std::optional<resp::Integer> decode_integer(std::istream &stream) {
        if (const auto value = decode_string(stream)) {
            try {
                return resp::Integer{std::stoi(*value)};
            } catch (...) {
                return std::nullopt;
            }
        }

        return std::nullopt;
    }

    std::optional<resp::BulkString> decode_bulk_string(std::istream &stream) {
        if (const auto length_string = decode_string(stream)) {
            try {
                const int length = std::stoi(*length_string);

                if (length == -1) {
                    return resp::BulkString{std::nullopt};
                }

                if (length < 0) {
                    return std::nullopt;
                }

                if (auto value = decode_string(stream, length)) {
                    return resp::BulkString{std::move(*value)};
                }
            } catch (...) {
                return std::nullopt;
            }
        }

        return std::nullopt;
    }

    std::optional<resp::Array> decode_array(std::istream &stream) {
        if (const auto length_string = decode_string(stream)) {
            try {
                const int length = std::stoi(*length_string);

                if (length == -1) {
                    return resp::Array{std::nullopt};
                }

                std::vector<resp::Message> messages{static_cast<size_t>(length)};
                for (int i{0}; i < length; ++i) {
                    if (auto message = resp::decode(stream)) {
                        messages[i] = std::move(*message);
                    } else {
                        return std::nullopt;
                    }
                }

                return resp::Array{std::move(messages)};
            } catch (...) {
                return std::nullopt;
            }
        }

        return std::nullopt;
    }

    std::string encode_simple_string(const resp::SimpleString &simple_string) {
        return std::format("+{}\r\n", simple_string.value);
    }

    std::string encode_simple_error(const resp::SimpleError &simple_error) {
        return std::format("-{}\r\n", simple_error.value);
    }

    std::string encode_integer(const resp::Integer &integer) {
        return std::format(":{}\r\n", integer.value);
    }

    std::string encode_bulk_string(const resp::BulkString &bulk_string) {
        if (!bulk_string.value.has_value()) {
            return "$-1\r\n";
        }
        return std::format("${}\r\n{}\r\n", bulk_string.value->size(), *bulk_string.value);
    }

    std::string encode_array(const resp::Array &array) {
        if (!array.value.has_value()) {
            return "*-1\r\n";
        }

        std::string encoded{std::format("*{}\r\n", array.value->size())};

        for (const resp::Message &message: *array.value) {
            encoded += encode(message);
        }

        return encoded;
    }
}

namespace resp {
    std::optional<Message> decode(std::istream &stream) {
        std::optional<Message> message{};
        switch (stream.get()) {
            case '+':
                message = decode_simple_string(stream);
                break;
            case '-':
                message = decode_simple_error(stream);
                break;
            case ':':
                message = decode_integer(stream);
                break;
            case '$':
                message = decode_bulk_string(stream);
                break;
            case '*':
                message = decode_array(stream);
                break;
            default:
                message = std::nullopt;
        }

        return message;
    }

    std::string encode(const Message &message) {
        return std::visit([](auto &&msg) -> std::string {
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
}






