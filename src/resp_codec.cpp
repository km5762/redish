//
// Created by d4wgr on 4/23/2025.
//

#include "resp_codec.h"

std::optional<RespCodec::Message> RespCodec::deserialize(std::string_view data) {
    if (data.empty()) {
        return std::nullopt;
    }

    switch (data[0]) {
        case '+':

        default:
            return std::nullopt;
    }
}

}

namespace {
    std::optional<std::string> deserialize_string(std::string_view data) {
    }
}
