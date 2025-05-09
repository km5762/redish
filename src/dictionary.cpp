//
// Created by d4wgr on 5/6/2025.
//

#include "dictionary.h"

#include <chrono>

std::optional<std::reference_wrapper<resp::Message> > Dictionary::get(const std::string &key) {
    std::lock_guard lock(m_mutex);

    if (!m_map.contains(key)) {
        return std::nullopt;
    }

    return m_map.at(key);
}

void Dictionary::set(const std::string &key, const resp::Message &value) {
    std::lock_guard lock(m_mutex);

    m_map[key] = value;
}

std::optional<std::reference_wrapper<resp::Message> > Dictionary::set_and_get(
    const std::string &key, const resp::Message &value) {
    std::lock_guard lock(m_mutex);

    std::optional<std::reference_wrapper<resp::Message> > previous = std::nullopt;

    if (m_map.contains(key)) {
        previous = m_map.at(key);
    }

    m_map[key] = value;
    return previous;
}
