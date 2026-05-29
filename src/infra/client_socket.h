#pragma once

#include <unistd.h>

#include <utility>

class ClientSocket {
public:
    ClientSocket() = default;
    explicit ClientSocket(int fd) : fd_{fd} {}

    ~ClientSocket() {
        reset();
    }

    ClientSocket(const ClientSocket&) = delete;
    ClientSocket& operator=(const ClientSocket&) = delete;

    ClientSocket(ClientSocket&& other) noexcept : fd_{other.fd_} {
        other.fd_ = -1;
    }

    ClientSocket& operator=(ClientSocket&& other) noexcept {
        if (this != &other) {
            reset();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    int get() const {
        return fd_;
    }

    explicit operator bool() const {
        return fd_ >= 0;
    }

    void reset(int new_fd = -1) {
        if (fd_ >= 0) {
            ::close(fd_);
        }
        fd_ = new_fd;
    }

private:
    int fd_{-1};
};
