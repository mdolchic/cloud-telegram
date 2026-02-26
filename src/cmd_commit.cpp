//
// Created by Максим Долганов on 7.01.26.
//

#include "protocol_internal.h"
#include "net_io.h"
#include "storage.h"

#include <filesystem>
#include <string>
#include <vector>

static std::string err_line(const std::string& m) {
    return "ERROR " + m + "\n";
}

bool cmd_commit(int fd, const std::string& uid, const std::string& album) {
    std::filesystem::path inbox{};
    std::string err{};
    if (!storage_user_inbox(uid, inbox, err)) {
        return net_io::write_all(fd, err_line(err));
    }

    std::filesystem::path album_dir{};
    if (!storage_album_dir(uid, album, album_dir, err)) {
        return net_io::write_all(fd, err_line(err));
    }

    std::vector<std::filesystem::path> metas{};
    storage_list_meta_files(inbox, metas);

    std::size_t moved{0};
    for (const auto& meta_src : metas) {
        auto bin_src = storage_bin_from_meta(meta_src);
        if (!std::filesystem::exists(bin_src)) {
            return net_io::write_all(fd, err_line("corrupt inbox"));
        }
        if (!storage_move_pair(bin_src, meta_src, album_dir, err)) {
            return net_io::write_all(fd, err_line(err));
        }
        moved += 1;
    }

    return net_io::write_all(fd, "OK " + std::to_string(moved) + "\n");
}