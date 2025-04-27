//
// Created by d4wgr on 4/23/2025.
//

#include "resp_codec.h"

#include <iostream>
#include <ostream>

std::optional<RespCodec::Message> RespCodec::decode(const std::string_view data) {
    m_data = data;
    std::optional<Message> message{};
    switch (advance()) {
        case '+':
            message = decode_simple_string();
            break;
        default:
            message = std::nullopt;
    }

    // if we have not consumed all the input
    if (m_position != data.size()) {
        return std::nullopt;
    }
    return message;
}

std::optional<RespCodec::SimpleString> RespCodec::decode_simple_string() {
    size_t i{m_position};

    while (true) {
        if (i + 1 >= m_data.size()) {
            return std::nullopt;
        }
        if (m_data[i] == '\r' && m_data[i + 1] == '\n') {
            break;
        }
        ++i;
    }

    const size_t size{i - m_position};
    const std::string s{m_data.data() + m_position, size};
    m_position = i + 2;
    return SimpleString{s};
}
