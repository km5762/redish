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
        setg(m_buffer.data(), m_buffer.data(), m_buffer.data());
    }

protected:
    int underflow() override {
        if (gptr() == egptr()) {
            const int bytes_read = recv(m_socket, m_buffer.data(), m_buffer.size(), 0);

            // socket no longer good
            if (bytes_read <= 0) {
                return traits_type::eof();
            }

            setg(m_buffer.data(), m_buffer.data(), m_buffer.data() + bytes_read);
        }

        return *gptr();
    }

private:
    std::array<char, N> m_buffer;
    int m_socket;
};

#endif //TCP_STREAM_ITERATOR_H
