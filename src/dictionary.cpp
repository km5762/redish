//
// Created by d4wgr on 5/6/2025.
//

#include "dictionary.h"

#include <cassert>
#include <chrono>
#include <expected>
#include <fstream>

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

ssize_t Dictionary::push(const std::string &key, const std::span<const resp::Value> values, const bool reverse) {
    const auto value = get(key);

    if (!value.has_value()) {
        m_map[key] = {
            resp::Array{
                reverse
                    ? std::vector<resp::Value>{values.rbegin(), values.rend()}
                    : std::vector<resp::Value>{values.begin(), values.end()},
            },
            std::nullopt,
        };
        return static_cast<ssize_t>(values.size());
    }

    auto *array = std::get_if<resp::Array>(&value->get());
    if (array == nullptr || !array->value.has_value()) {
        return -1;
    }

    if (reverse) {
        array->value->insert(array->value->begin(), values.rbegin(), values.rend());
    } else {
        array->value->insert(array->value->end(), values.begin(), values.end());
    }

    return static_cast<ssize_t>(array->value->size());
}

std::optional<std::span<const resp::Value> >
Dictionary::range(const std::string &key, ptrdiff_t start, ptrdiff_t stop) {
    const auto value = get(key);

    if (!value.has_value()) {
        return std::span<const resp::Value>{};
    }

    auto *array = std::get_if<resp::Array>(&value->get());
    if (array == nullptr || !array->value.has_value()) {
        return std::nullopt;
    }

    const auto size = static_cast<ptrdiff_t>(array->value->size());

    if (start < 0) {
        start = size + start;
    }

    if (stop < 0) {
        stop = size + stop;
    }

    if (size == 0 || stop < start) {
        return std::span<const resp::Value>{};
    }

    start = std::clamp(start, ptrdiff_t{0}, size - 1);
    stop = std::clamp(stop, start, size - 1);

    return std::span{array->value->data() + start, static_cast<size_t>(stop - start + 1)};
}

std::expected<int64_t, Dictionary::incr_error> Dictionary::incr(const std::string &key, const int64_t amount) {
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

        return previous + amount;
    }

    set(key, resp::BulkString{std::to_string(previous + amount)});
    return previous + amount;
}

void Dictionary::save(std::ostream &stream) const {
    const size_t size{m_map.size()};
    stream.write(reinterpret_cast<const char *>(&size), sizeof(size));
    for (const auto &[key, value]: m_map) {
        const auto key_size{static_cast<std::streamsize>(key.size())};
        stream.write(reinterpret_cast<const char *>(&key_size), sizeof(key_size));
        stream.write(key.data(), key_size);

        resp::save(value.first, stream);

        const auto timestamp = value.second;
        const bool has_timestamp = timestamp.has_value();
        stream.write(reinterpret_cast<const char *>(&has_timestamp), sizeof(bool));

        if (has_timestamp) {
            const auto count = timestamp->time_since_epoch().count();
            stream.write(reinterpret_cast<const char *>(&count), sizeof(count));
        }
    }
}

void Dictionary::load(std::istream &stream) {
    size_t size{};
    stream.read(reinterpret_cast<char *>(&size), sizeof(size));

    for (size_t i{0}; i < size; ++i) {
        std::streamsize key_size{};
        stream.read(reinterpret_cast<char *>(&key_size), sizeof(key_size));
        std::string key(key_size, '\0');
        stream.read(key.data(), key_size);
        const resp::Value value{resp::load(stream)};
        bool has_timestamp{};
        stream.read(reinterpret_cast<char *>(&has_timestamp), sizeof(bool));
        std::optional<Timestamp> timestamp{std::nullopt};
        if (has_timestamp) {
            Clock::duration::rep count{};
            stream.read(reinterpret_cast<char *>(&count), sizeof(count));
            timestamp = Timestamp{Clock::duration{count}};
        }
        set(key, value, timestamp);
    }
}

bool Dictionary::expired(const std::string &key) const {
    if (!m_map.contains(key)) {
        return false;
    }

    const auto timestamp = m_map.at(key).second;

    return timestamp.has_value() && Clock::now() >= *timestamp;
}

