#include "resp.h"

#include <assert.h>
#include <charconv>
#include <istream>
#include <ostream>


namespace {
    using namespace resp;

    void append(std::vector<char> &out, const std::string_view str) {
        out.insert(out.end(), str.begin(), str.end());
    }

    void append_crlf(std::vector<char> &out) {
        out.push_back('\r');
        out.push_back('\n');
    }

    void append_int(std::vector<char> &out, const int64_t val) {
        char buf[21]; // enough for 64-bit int + sign
        auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), val);
        out.insert(out.end(), buf, ptr);
    }

    void serialize(const SimpleString &s, std::vector<char> &out) {
        out.push_back('+');
        append(out, s.value);
        append_crlf(out);
    }

    void serialize(const SimpleError &e, std::vector<char> &out) {
        out.push_back('-');
        append(out, e.prefix);
        append(out, " ");
        append(out, e.value);
        append_crlf(out);
    }

    void serialize(const Integer &i, std::vector<char> &out) {
        out.push_back(':');
        append_int(out, i.value);
        append_crlf(out);
    }

    void serialize(const BulkString &b, std::vector<char> &out) {
        if (!b.value) {
            append(out, "$-1\r\n");
            return;
        }
        append(out, "$");
        append_int(out, static_cast<int64_t>(b.value->size()));
        append_crlf(out);
        append(out, *b.value);
        append_crlf(out);
    }

    void serialize(const Array &a, std::vector<char> &out) {
        if (!a.value) {
            append(out, "*-1\r\n");
            return;
        }
        append(out, "*");
        append_int(out, static_cast<int64_t>(a.value->size()));
        append_crlf(out);
        for (const auto &v: *a.value) {
            serialize(v, out);
        }
    }

    void save_int(const int64_t value, std::ostream &out) {
        out.write(reinterpret_cast<const char *>(&value), sizeof(value));
    }

    void save_string(const std::string_view string, std::ostream &out) {
        const int64_t size{static_cast<int64_t>(string.size())};
        save_int(size, out);
        out.write(string.data(), size);
    }

    void save(const SimpleString &string, std::ostream &out) {
        save_string(string.value, out);
    }

    void save(const SimpleError &error, std::ostream &out) {
        save_string(error.prefix, out);
        save_string(error.value, out);
    }

    void save(const Integer &integer, std::ostream &out) {
        save_int(integer.value, out);
    }

    void save(const BulkString &string, std::ostream &out) {
        if (!string.value.has_value()) {
            save_int(-1, out);
            return;
        }

        save_string(*string.value, out);
    }

    void save(const Array &array, std::ostream &out) {
        if (!array.value.has_value()) {
            save_int(-1, out);
        }
        const int64_t size{static_cast<int64_t>(array.value->size())};
        save_int(size, out);
        for (const Value &val: *array.value) {
            save(val, out);
        }
    }

    template<typename T, typename V, size_t I = 0>
    constexpr size_t variant_index() {
        if constexpr (I >= std::variant_size_v<V>) {
            return std::variant_size_v<V>;
        } else if constexpr (std::is_same_v<std::variant_alternative_t<I, V>, T>) {
            return I;
        } else {
            return variant_index<T, V, I + 1>();
        }
    }

    int64_t load_int(std::istream &in) {
        int64_t result{};
        in.read(reinterpret_cast<char *>(&result), sizeof(result));
        return result;
    }

    std::optional<std::string> load_string(std::istream &in) {
        const int64_t size{load_int(in)};

        if (size == -1) {
            return std::nullopt;
        }

        assert(size > 0);

        std::string result(size, '\0');
        in.read(result.data(), size);
        return result;
    }

    SimpleString load_simple_string(std::istream &in) {
        return {*load_string(in)};
    }

    SimpleError load_simple_error(std::istream &in) {
        return {*load_string(in), *load_string(in)};
    }

    Integer load_integer(std::istream &in) {
        return {load_int(in)};
    }

    BulkString load_bulk_string(std::istream &in) {
        return {load_string(in)};
    }

    Array load_array(std::istream &in) {
        std::vector<Value> result{};

        const int64_t size{load_int(in)};

        if (size == -1) {
            return {std::nullopt};
        }

        assert(size > 0);
        result.reserve(size);

        for (size_t i{0}; i < size; ++i) {
            result.emplace_back(load(in));
        }

        return {std::move(result)};
    }
}

namespace resp {
    void serialize(const Value &value, std::vector<char> &out) {
        std::visit([&out](auto &&arg) {
            ::serialize(arg, out);
        }, value);
    }

    void save(const Value &value, std::ostream &out) {
        const size_t tag = value.index();
        out.write(reinterpret_cast<const char *>(&tag), sizeof(tag));
        std::visit([&out](const auto &v) {
            ::save(v, out);
        }, value);
    }

    Value load(std::istream &in) {
        size_t tag{};
        in.read(reinterpret_cast<char *>(&tag), sizeof(tag));

        switch (tag) {
            case variant_index<SimpleString, Value>():
                return load_simple_string(in);
            case variant_index<SimpleError, Value>():
                return load_simple_error(in);
            case variant_index<Integer, Value>():
                return load_integer(in);
            case variant_index<BulkString, Value>():
                return load_bulk_string(in);
            case variant_index<Array, Value>():
                return load_array(in);
            default:
                throw std::runtime_error("resp::load: invalid tag encountered");
        }
    }
}
