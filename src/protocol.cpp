//
// Created by Максим Долганов on 6.01.26.
//

#include "protocol.h"
#include "net_io.h"
#include "protocol_internal.h"

#include <sstream>
#include <string>

static std::string ok_line() { return "OK\n"; }
static std::string err_line(const std::string& m) { return "ERROR " + m + "\n"; }

bool handle_command(int fd, const std::string& line) {
    std::istringstream iss{line};
    std::string cmd{};
    iss >> cmd;
    if (cmd.empty()) {
        return net_io::write_all(fd, err_line("empty_command"));
    }
    if (cmd == "PING") {
        return net_io::write_all(fd, ok_line());
    }
    if (cmd == "EXIT") {
        return net_io::write_all(fd, ok_line());
    }

    if (cmd == "ADDF") {
        std::string uid{};
        char kind{};
        std::size_t name_len{};
        std::size_t nbytes{};
        if (!(iss >> uid >> kind >> name_len >> nbytes)) {
            return net_io::write_all(fd, err_line("usage: ADDF <uid> <kind> <name_len> <nbytes>"));
        }
        return cmd_addf(fd, uid, kind, name_len, nbytes);
    }

    if (cmd == "COMMIT") {
        std::string uid{}, album{};
        if (!(iss >> uid >> album)) {
            return net_io::write_all(fd, err_line("usage: COMMIT <uid> <album>"));
        }
        return cmd_commit(fd, uid, album);
    }

    if (cmd == "LISTALBUMS") {
        std::string uid{};
        if (!(iss >> uid)) {
            return net_io::write_all(fd, err_line("usage: LISTALBUMS <uid>"));
        }
        return cmd_listalbums(fd, uid);
    }

    if (cmd == "GETALBUM") {
        std::string uid{}, album{};
        if (!(iss >> uid >> album)) {
            return net_io::write_all(fd, err_line("usage: GETALBUM <uid> <album>"));
        }
        return cmd_getalbum(fd, uid, album);
    }

    return net_io::write_all(fd, err_line("unknown_command"));
}