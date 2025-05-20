//
// Created by d4wgr on 5/6/2025.
//

#include "dictionary.h"

#include <chrono>

std::optional<std::reference_wrapper<resp::Value> > Dictionary::get(const std::string &key) {
    if (!exists(key)) {
        return std::nullopt;
    }

    if (expired(key)) {
        m_map.erase(key);
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

bool Dictionary::exists(const std::string &key) const {
    return m_map.contains(key);
}

bool Dictionary::expired(const std::string &key) const {
    if (!exists(key)) {
        return false;
    }

    const auto timestamp = m_map.at(key).second;

    return timestamp.has_value() && Clock::now() >= *timestamp;
}

