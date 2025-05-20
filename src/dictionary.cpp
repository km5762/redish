//
// Created by d4wgr on 5/6/2025.
//

#include "dictionary.h"

#include <cassert>
#include <chrono>
#include <expected>

std::optional<std::reference_wrapper<resp::Value> > Dictionary::get(const std::string &key) {
    if (!exists(key)) {
        return std::nullopt;
    }

    return m_map.at(key).first;
}

void Dictionary::set(const std::string &key, const resp::Value &value, const std::optional<Timestamp> &expiry) {
    m_map[key] = {value, expiry};
}

std::optional<resp::Value> Dictionary::set_and_get(
    const std::string &key, const resp::Value &value, const std::optional<Timestamp> &expiry) {
    std::optional<resp::Value> previous = get(key);

    set(key, value, expiry);
    return previous;
}

void Dictionary::flush() {
    m_map.clear();
}

bool Dictionary::exists(const std::string &key) {
    if (!m_map.contains(key)) {
        return false;
    }

    if (expired(key)) {
        m_map.erase(key);
        return false;
    }

    return true;
}

void Dictionary::del(const std::string &key) {
    m_map.erase(key);
}

std::expected<int64_t, Dictionary::incr_error> Dictionary::incr(const std::string &key) {
    int64_t previous{0};

    if (const auto &value = get(key)) {
        if (!std::holds_alternative<resp::BulkString>(value->get())) {
            return std::unexpected{incr_error::non_bulk_string_value};
        }

        std::optional<std::string> &string = std::get<resp::BulkString>(value->get()).value;

        if (!string.has_value()) {
            return std::unexpected{incr_error::null_bulk_string_value};
        }

        auto [ptr, ec] = std::from_chars(string->data(), string->data() + string->size(), previous);

        if (ec != std::errc() || ptr != string->data() + string->size()) {
            return std::unexpected{incr_error::non_numeric_value};
        }

        // enough for 64-bit integer
        string->resize_and_overwrite(20, [&](char *buffer, size_t) -> size_t {
            auto [ptr, ec] = std::to_chars(buffer, buffer + 20, previous + 1);
            assert(ec == std::errc()); // this should never fail (in theory)
            return static_cast<size_t>(ptr - buffer); // number of chars written
        });

        return previous + 1;
    }

    set(key, resp::BulkString{std::to_string(previous + 1)});
    return previous + 1;
}

bool Dictionary::expired(const std::string &key) const {
    if (!m_map.contains(key)) {
        return false;
    }

    const auto timestamp = m_map.at(key).second;

    return timestamp.has_value() && Clock::now() >= *timestamp;
}

