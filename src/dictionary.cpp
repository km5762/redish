//
// Created by d4wgr on 5/6/2025.
//

#include "dictionary.h"

#include <chrono>

std::optional<std::reference_wrapper<resp::Value> > Dictionary::get(const std::string &key) {
    if (!m_map.contains(key)) {
        return std::nullopt;
    }

    return m_map.at(key);
}

void Dictionary::set(const std::string &key, const resp::Value &value) {
    m_map[key] = value;
}

std::optional<resp::Value> Dictionary::set_and_get(
    const std::string &key, const resp::Value &value) {
    std::optional<resp::Value> previous = std::nullopt;

    if (m_map.contains(key)) {
        previous = m_map.at(key);
    }

    m_map[key] = value;
    return previous;
}

void Dictionary::flush() {
    m_map.clear();
}

bool Dictionary::contains(const std::string &key) const {
    return m_map.contains(key);
}
