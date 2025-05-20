//
// Created by d4wgr on 5/19/2025.
//

#ifndef TOKENIZER_H
#define TOKENIZER_H
#include <iterator>

#include "resp.h"


class Tokenizer {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::optional<std::string_view>;
    using difference_type = std::ptrdiff_t;
    using reference = std::optional<std::string_view>;
    using pointer = const std::optional<std::string_view> *;

    explicit Tokenizer(const resp::Array &array)
        : m_array(array) {
    }

    class Iterator {
    public:
        Iterator(const resp::Array &array, const size_t index)
            : m_array(array), m_index(index) {
        }

        std::optional<std::string_view> operator*() const {
            if (!m_array.value || m_index >= m_array.value->size())
                return std::nullopt;

            const auto &value = (*m_array.value)[m_index];
            const auto *bulk = std::get_if<resp::BulkString>(&value);
            if (!bulk || !bulk->value.has_value())
                return std::nullopt;

            return std::string_view{*bulk->value};
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
        const resp::Array &m_array;
        size_t m_index;
    };

    [[nodiscard]] Iterator begin() const { return {m_array, 0}; }
    [[nodiscard]] Iterator end() const { return {m_array, m_array.value.has_value() ? m_array.value->size() : 0}; }

private:
    const resp::Array &m_array;
};


#endif //TOKENIZER_H
