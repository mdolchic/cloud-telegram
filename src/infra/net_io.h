//
// Created by Максим Долганов on 6.01.26.
//

#pragma once

#include <sys/socket.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <string>

namespace net_io {

inline bool read_line(int fd, std::string& out, std::size_t max_len = 1000000) {
    out.clear();
    while (true) {
        char c;
        const ssize_t n = ::recv(fd, &c, 1, 0);
        if (n == 0) {
            return false;
        }
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (c == '\n') {
            break;
        }
        if (c != '\r') {
            out.push_back(c);
        }
        if (out.size() > max_len) {
            return false;
        }
    }
    return true;
}

inline bool read_exact(int fd, void* buf, std::size_t n) {
    std::uint8_t* p = static_cast<std::uint8_t*>(buf);
    std::size_t left = n;
    while (left > 0) {
        const ssize_t r = ::recv(fd, p, left, 0);
        if (r == 0) {
            return false;
        }
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        p += static_cast<std::size_t>(r);
        left -= static_cast<std::size_t>(r);
    }
    return true;
}

inline bool write_all(int fd, const std::string& s) {
    const char* p = s.data();
    std::size_t left = s.size();
    while (left > 0) {
        const ssize_t w = ::send(fd, p, left, 0);
        if (w < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        p += static_cast<std::size_t>(w);
        left -= static_cast<std::size_t>(w);
    }
    return true;
}

inline bool write_exact(int fd, const void* buf, std::size_t n) {
    const std::uint8_t* p = static_cast<const std::uint8_t*>(buf);
    std::size_t left = n;
    while (left > 0) {
        const ssize_t w = ::send(fd, p, left, 0);
        if (w < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        p += static_cast<std::size_t>(w);
        left -= static_cast<std::size_t>(w);
    }
    return true;
}

}
