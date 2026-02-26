//
// Created by Максим Долганов on 7.01.26.
//

#include "protocol_internal.h"
#include "net_io.h"
#include "storage.h"
#include "id_gen.h"
#include "meta.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

static constexpr std::size_t kMaxFileBytes{25ull * 1024ull * 1024ull};

static std::string err_line(const std::string& m) {
    return "ERROR " + m + "\n";
}
static std::string ok_id(const std::string& id) {
    return "OK " + id + "\n";
}

bool cmd_addf(int fd, const std::string& uid, char kind, std::size_t name_len, std::size_t nbytes) {
    if (!(kind == 'P' || kind == 'D')) {
        return net_io::write_all(fd, err_line("bad kind"));
    }
    if (name_len > 4096) {
        return net_io::write_all(fd, err_line("name too long"));
    }
    if (nbytes == 0 || nbytes > kMaxFileBytes) {
        return net_io::write_all(fd, err_line("bad file size"));
    }

    std::filesystem::path inbox{};
    std::string err{};
    if (!storage_user_inbox(uid, inbox, err)) {
        return net_io::write_all(fd, err_line(err));
    }

    std::string name{};
    name.resize(name_len);
    if (name_len != 0) { if (!net_io::read_exact(fd, name.data(), name_len)) {
        return false;
    } }

    std::vector<std::uint8_t> data(nbytes);
    if (!net_io::read_exact(fd, data.data(), nbytes)) {
        return false;
    }

    std::string id{id_gen_make()};
    auto bin_path = inbox / (id + ".bin");
    auto meta_path = inbox / (id + ".meta");

    {
        std::ofstream out{bin_path, std::ios::binary};
        if (!out) {
            return net_io::write_all(fd, err_line("failed to create file"));
        }
        out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        if (!out) {
            return net_io::write_all(fd, err_line("failed to write file"));
        }
        out.flush();
        if (!out) {
            return net_io::write_all(fd, err_line("failed to flush file"));
        }
    }

    if (!meta_write(meta_path, kind, name)) {
        std::error_code ec{};
        std::filesystem::remove(bin_path, ec);
        return net_io::write_all(fd, err_line("failed to write meta"));
    }

    return net_io::write_all(fd, ok_id(id));
}