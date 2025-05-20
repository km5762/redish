//
// Created by d4wgr on 5/19/2025.
//

#ifndef TOKENIZER_H
#define TOKENIZER_H
#include <array>
#include <iterator>
#include <strings.h>

#include "resp.h"

class Tokenizer {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::optional<std::string_view>;
    using difference_type = std::ptrdiff_t;
    using reference = std::optional<std::string_view>;
    using pointer = const std::optional<std::string_view> *;

    explicit Tokenizer(const std::vector<resp::Value> &array)
        : m_array(array) {
    }

    [[nodiscard]] std::optional<std::string_view> get_string(const size_t index) const {
        return extract_string_view(m_array, index);
    }

    [[nodiscard]] const resp::Value &get_value(const size_t index) const {
        return m_array[index];
    }

    [[nodiscard]] std::size_t size() const {
        return m_array.size();
    }

    class Iterator {
    public:
        Iterator(const std::vector<resp::Value> &array, const size_t index)
            : m_array(array), m_index(index) {
        }

        std::optional<std::string_view> operator*() const {
            return extract_string_view(m_array, m_index);
        }

        Iterator &operator++() {
            ++m_index;
            return *this;
        }

        Iterator operator++(int) {
            const Iterator temp = *this;
            ++(*this);
            return temp;
        }

        friend bool operator==(const Iterator &a, const Iterator &b) {
            return &a.m_array == &b.m_array && a.m_index == b.m_index;
        }

        friend bool operator!=(const Iterator &a, const Iterator &b) {
            return !(a == b);
        }

    private:
        const std::vector<resp::Value> &m_array{};
        size_t m_index;
    };

    [[nodiscard]] Iterator begin() const { return {m_array, 0}; }

    [[nodiscard]] Iterator end() const { return {m_array, m_array.size()}; }

private
:
    const std::vector<resp::Value> &m_array{};

    static std::optional<std::string_view> extract_string_view(const std::vector<resp::Value> &array, size_t index) {
        if (index >= array.size()) {
            return std::nullopt;
        }

        const auto &value = array[index];
        const auto *bulk = std::get_if<resp::BulkString>(&value);
        if (!bulk || !bulk->value.has_value()) {
            return std::nullopt;
        }

        return std::string_view{*bulk->value};
    }
};


#endif //TOKENIZER_H
