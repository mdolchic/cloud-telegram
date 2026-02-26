//
// Created by Максим Долганов on 7.01.26.
//

#include "item_io.h"
#include "net_io.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

static constexpr std::size_t kMaxFileBytes{25ull * 1024ull * 1024ull};
static constexpr std::size_t kChunk{64ull * 1024ull};

bool item_send(int fd, char kind, const std::string& name, const std::filesystem::path& bin_path) {
    std::ifstream in{bin_path, std::ios::binary};
    if (!in) {
        return net_io::write_all(fd, "ERROR file not found\n");
    }

    in.seekg(0, std::ios::end);
    auto end = in.tellg();
    if (end < 0) {
        return net_io::write_all(fd, "ERROR size error\n");
    }

    std::size_t nbytes{static_cast<std::size_t>(end)};
    if (nbytes > kMaxFileBytes) {
        return net_io::write_all(fd, "ERROR file too large\n");
    }
    in.seekg(0, std::ios::beg);

    std::string header{"ITEM "};
    header.push_back(kind);
    header += " " + std::to_string(name.size()) + " " + std::to_string(nbytes) + "\n";

    if (!net_io::write_all(fd, header)) {
        return false;
    }
    if (!name.empty()) {
        if (!net_io::write_exact(fd, name.data(), name.size())) {
            return false;
        }
    }

    std::vector<std::uint8_t> buf(kChunk);
    std::size_t left{nbytes};
    while (left > 0) {
        std::size_t take{left < buf.size() ? left : buf.size()};
        in.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(take));
        if (in.gcount() != static_cast<std::streamsize>(take)) {
            return net_io::write_all(fd, "ERROR read failed\n");
        }
        if (!net_io::write_exact(fd, buf.data(), take)) {
            return false;
        }
        left -= take;
    }
    return true;
}