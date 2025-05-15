#include "resp.h"
#include <charconv>

namespace resp {
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
        append_int(out, b.value->size());
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
        append_int(out, a.value->size());
        append_crlf(out);
        for (const auto &v: *a.value) {
            serialize(v, out);
        }
    }

    void serialize(const Value &value, std::vector<char> &out) {
        std::visit([&out](auto &&arg) {
            serialize(arg, out);
        }, value);
    }
}
