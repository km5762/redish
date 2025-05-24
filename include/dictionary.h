//
// Created by d4wgr on 5/6/2025.
//

#ifndef DICTIONARY_H
#define DICTIONARY_H
#include <expected>
#include <mutex>
#include <unordered_map>

#include "resp_parser.h"

using Clock = std::chrono::system_clock;
using Timestamp = std::chrono::time_point<Clock>;

class Dictionary {
public:
    std::optional<std::reference_wrapper<resp::Value> > get(const std::string &key);

    void set(const std::string &key, const resp::Value &value, const std::optional<Timestamp> &expiry = std::nullopt);

    std::optional<resp::Value> set_and_get(const std::string &key,
                                           const resp::Value &value,
                                           const std::optional<Timestamp> &expiry = std::nullopt);

    void flush();

    bool exists(const std::string &key);

    void del(const std::string &key);

    ssize_t push(const std::string &key, std::span<const resp::Value> values, bool reverse = false);

    std::optional<std::span<const resp::Value> > range(const std::string &key, ptrdiff_t start, ptrdiff_t stop);

    enum class incr_error {
        non_bulk_string_value,
        null_bulk_string_value,
        non_numeric_value,
    };

    std::expected<int64_t, incr_error> incr(const std::string &key, int64_t amount = 1);

    void save(std::ostream &stream) const;

    void load(std::istream &stream);

private:
    std::unordered_map<std::string, std::pair<resp::Value, std::optional<Timestamp> > > m_map{};

    bool expired(const std::string &key) const;
};


#endif //DICTIONARY_H
