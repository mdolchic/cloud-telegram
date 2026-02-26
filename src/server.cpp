//
// Created by Максим Долганов on 6.01.26.
//

#include "net_io.h"
#include "protocol.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

namespace {
    constexpr int kPort{5555};
}

int main() {
    int srv{::socket(AF_INET, SOCK_STREAM, 0)};
    if (srv < 0) {
        std::cerr << "socket() failed: " << std::strerror(errno) << "\n";
        return 1;
    }

    int yes{1};
    ::setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<std::uint16_t>(kPort));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (::bind(srv, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "bind() failed: " << std::strerror(errno) << "\n";
        ::close(srv);
        return 1;
    }

    if (::listen(srv, 16) < 0) {
        std::cerr << "listen() failed: " << std::strerror(errno) << "\n";
        ::close(srv);
        return 1;
    }

    std::cerr << "Server listening on 127.0.0.1:" << kPort << "\n";

    while (true) {
        int cli{::accept(srv, nullptr, nullptr)};
        if (cli < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "accept() failed: " << std::strerror(errno) << "\n";
            break;
        }

        std::string line{};
        while (net_io::read_line(cli, line)) {
            if (!handle_command(cli, line)) {
                break;
            }
            if (line == "EXIT") {
                break;
            }
        }

        ::close(cli);
    }

    ::close(srv);
    return 0;
}