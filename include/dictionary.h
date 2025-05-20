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

    enum class incr_error {
        non_bulk_string_value,
        null_bulk_string_value,
        non_numeric_value,
    };

    std::expected<int64_t, incr_error> incr(const std::string &key);

private:
    std::unordered_map<std::string, std::pair<resp::Value, std::optional<Timestamp> > > m_map{};

    bool expired(const std::string &key) const;
};


#endif //DICTIONARY_H
