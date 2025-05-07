//
// Created by d4wgr on 5/4/2025.
//

#ifndef TCP_STREAM_ITERATOR_H
#define TCP_STREAM_ITERATOR_H
#include <array>
#include <cstddef>
#include <streambuf>
#include <string>
#include <sys/socket.h>

template<size_t N = 4096>
class TcpStreambuf final : public std::streambuf {
public:
    explicit TcpStreambuf(const int socket): m_socket(socket) {
        // sets the current pointer to the end of the stream. next call will recv data into buffer
        setg(m_read_buffer.data(), m_read_buffer.data(), m_read_buffer.data());
        // set the write buffer to the whole stream
        setp(m_write_buffer.data(), m_write_buffer.data() + m_write_buffer.size());
    }

    ~TcpStreambuf() override {
        sync();
    }

protected:
    int underflow() override {
        if (gptr() == egptr()) {
            const int bytes_read = recv(m_socket, m_read_buffer.data(), m_read_buffer.size(), 0);

            // socket no longer good
            if (bytes_read <= 0) {
                // invalidate gptr
                setg(m_read_buffer.data(), nullptr, m_read_buffer.data());
                return traits_type::eof();
            }

            setg(m_read_buffer.data(), m_read_buffer.data(), m_read_buffer.data() + bytes_read);
        }

        return *gptr();
    }

    int overflow(int c) override {
        // if we are at the end, sync first then write
        if (pptr() == epptr()) {
            if (sync() == -1) {
                return traits_type::eof();
            }
            if (!std::char_traits<int>::eq_int_type(c, traits_type::eof())) {
                *pptr() = c;
                pbump(1);
            }
        } else {
            // otherwise, write first then sync
            if (!std::char_traits<int>::eq_int_type(c, traits_type::eof())) {
                *pptr() = c;
            }
            if (sync() == -1) {
                return traits_type::eof();
            }
        }


        if (std::char_traits<int>::eq_int_type(c, traits_type::eof())) {
            return std::char_traits<int>::not_eof(c);
        }
        return c;
    }

    int sync() override {
        const int len = pptr() - pbase();
        if (len > 0) {
            int total_bytes_sent = 0;
            int bytes_left = len;

            while (total_bytes_sent < len) {
                const int bytes_sent = send(m_socket, m_write_buffer.data() + total_bytes_sent, bytes_left, 0);
                if (bytes_sent == -1) {
                    return -1; // partial send or error
                }
                total_bytes_sent += bytes_sent;
                bytes_left -= bytes_sent;
            }
        }
        setp(m_write_buffer.data(), m_write_buffer.data() + m_write_buffer.size());
        return 0;
    }

private:
    std::array<char, N> m_read_buffer;
    std::array<char, N> m_write_buffer;
    int m_socket;
};

#endif //TCP_STREAM_ITERATOR_H
