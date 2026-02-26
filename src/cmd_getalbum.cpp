//
// Created by Максим Долганов on 7.01.26.
//

#include "protocol_internal.h"
#include "net_io.h"
#include "storage.h"
#include "meta.h"
#include "item_io.h"

#include <filesystem>
#include <string>
#include <vector>

static std::string err_line(const std::string& m) {
    return "ERROR " + m + "\n";
}

bool cmd_getalbum(int fd, const std::string& uid, const std::string& album) {
    std::filesystem::path album_dir{};
    std::string err{};
    if (!storage_album_dir(uid, album, album_dir, err)) {
        return net_io::write_all(fd, err_line(err));
    }

    std::vector<std::filesystem::path> metas{};
    storage_list_meta_files(album_dir, metas);

    if (!net_io::write_all(fd, "OK " + std::to_string(metas.size()) + "\n")) {
        return false;
    }

    for (const auto& meta_path : metas) {
        auto bin_path = storage_bin_from_meta(meta_path);
        if (!std::filesystem::exists(bin_path)) {
            return net_io::write_all(fd, err_line("corrupt album"));
        }

        char kind{};
        std::string name{};
        if (!meta_read(meta_path, kind, name)) {
            return net_io::write_all(fd, err_line("corrupt meta"));
        }
        if (!item_send(fd, kind, name, bin_path)) {
            return false;
        }
    }

    return net_io::write_all(fd, "END\n");
}