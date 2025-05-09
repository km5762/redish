//
// Created by d4wgr on 5/6/2025.
//

#ifndef DICTIONARY_H
#define DICTIONARY_H
#include <mutex>
#include <unordered_map>

#include "resp.h"


class Dictionary {
public:
    std::optional<std::reference_wrapper<resp::Message> > get(const std::string &key);

    void set(const std::string &key, const resp::Message &value);

    std::optional<std::reference_wrapper<resp::Message> > set_and_get(const std::string &key,
                                                                      const resp::Message &value);

private:
    std::unordered_map<std::string, resp::Message> m_map{};
    std::mutex m_mutex{};
};


#endif //DICTIONARY_H
