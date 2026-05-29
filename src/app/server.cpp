//
// Created by Максим Долганов on 6.01.26.
//

#include "app/http_session.h"
#include "infra/client_socket.h"
#include "infra/thread_pool.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <thread>

namespace {
    constexpr int kPort{5555};

    std::size_t worker_count() {
        const auto count = std::thread::hardware_concurrency();
        return count == 0 ? 4u : static_cast<std::size_t>(count);
    }
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

    ThreadPool pool{worker_count()};

    while (true) {
        int cli{::accept(srv, nullptr, nullptr)};
        if (cli < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "accept() failed: " << std::strerror(errno) << "\n";
            break;
        }

        try {
            pool.submit([client_fd = cli]() {
                try {
                    http_handle_client(ClientSocket{client_fd});
                } catch (const std::exception& e) {
                    std::cerr << "client handler error: " << e.what() << "\n";
                } catch (...) {
                    std::cerr << "client handler error: unknown exception\n";
                }
            });
        } catch (const std::exception& e) {
            std::cerr << "failed to submit client task: " << e.what() << "\n";
            ::close(cli);
        }
    }

    ::close(srv);
    return 0;
}
