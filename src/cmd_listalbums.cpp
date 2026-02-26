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

bool cmd_listalbums(int fd, const std::string& uid) {
    std::filesystem::path root{};
    std::string err{};
    if (!storage_user_albums_root(uid, root, err)) {
        return net_io::write_all(fd, err_line(err));
    }

    std::vector<std::string> names{};
    storage_list_album_names(root, names);

    if (!net_io::write_all(fd, "OK " + std::to_string(names.size()) + "\n")) {
        return false;
    }
    for (const auto& a : names) { if (!net_io::write_all(fd, a + "\n")) {
        return false;
    } }
    return net_io::write_all(fd, "END\n");
}