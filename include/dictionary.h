//
// Created by d4wgr on 5/6/2025.
//

#ifndef DICTIONARY_H
#define DICTIONARY_H
#include <mutex>
#include <unordered_map>

#include "resp_parser.h"


class Dictionary {
public:
    std::optional<std::reference_wrapper<resp::Value> > get(const std::string &key);

    void set(const std::string &key, const resp::Value &value);

    std::optional<resp::Value> set_and_get(const std::string &key,
                                           const resp::Value &value);

    void flush();

    bool contains(const std::string &key) const;

private:
    std::unordered_map<std::string, resp::Value> m_map{};
};


#endif //DICTIONARY_H
