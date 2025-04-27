//
// Created by d4wgr on 4/23/2025.
//

#include "resp_codec.h"

#include <iostream>
#include <ostream>

std::optional<RespCodec::Message> RespCodec::decode(const std::string_view data) {
    m_data = data;
    switch (advance()) {
        case '+':
            return decode_simple_string();
        default:
            return std::nullopt;
    }
}

std::optional<RespCodec::SimpleString> RespCodec::decode_simple_string() {
    size_t i{0};
    while (!(m_data[i] == '\r' && m_data[i + 1] == '\n')) {
        ++i;
    }

    const size_t size{i - m_position};
    const std::string s{m_data.data() + m_position, size};
    return SimpleString{s};
}
