//
// Created by Максим Долганов on 7.01.26.
//

#include "domain/storage.h"

#include <algorithm>
#include <system_error>

namespace {
    std::mutex g_storage_mutex{};
}

static std::filesystem::path root_dir() {
    return std::filesystem::path{"./data/users"};
}

static bool ensure_dir(const std::filesystem::path& p, std::string& err) {
    std::error_code ec{};
    std::filesystem::create_directories(p, ec);
    if (ec) {
        err = "create_directories failed: " + ec.message();
        return false;
    }
    return true;
}

static bool check_uid(const std::string& uid, std::string& err) {
    if (uid.empty()) {
        err = "uid is empty";
        return false;
    }
    if (uid.find("..") != std::string::npos) {
        err = "bad uid";
        return false;
    }
    if (uid.find('/') != std::string::npos) {
        err = "bad uid";
        return false;
    }
    if (uid.find('\\') != std::string::npos) {
        err = "bad uid";
        return false;
    }
    return true;
}

static bool check_album(const std::string& album, std::string& err) {
    if (album.empty()) {
        err = "album is empty";
        return false;
    }
    if (album.find("..") != std::string::npos) {
        err = "bad album";
        return false;
    }
    if (album.find('/') != std::string::npos) {
        err = "bad album";
        return false;
    }
    if (album.find('\\') != std::string::npos) {
        err = "bad album";
        return false;
    }
    if (album.find(':') != std::string::npos) {
        err = "bad album";
        return false;
    }
    return true;
}

bool storage_user_inbox(const std::string& uid, std::filesystem::path& out, std::string& err) {
    if (!check_uid(uid, err)) {
        return false;
    }
    out = root_dir() / uid / "inbox";
    return ensure_dir(out, err);
}

bool storage_user_albums_root(const std::string& uid, std::filesystem::path& out, std::string& err) {
    if (!check_uid(uid, err)) {
        return false;
    }
    out = root_dir() / uid / "albums";
    return ensure_dir(out, err);
}

bool storage_album_dir(const std::string& uid, const std::string& album, std::filesystem::path& out, std::string& err) {
    std::filesystem::path root{};
    if (!storage_user_albums_root(uid, root, err)) {
        return false;
    }
    if (!check_album(album, err)) {
        return false;
    }
    out = root / album;
    return ensure_dir(out, err);
}

bool storage_list_meta_files(const std::filesystem::path& dir, std::vector<std::filesystem::path>& metas) {
    metas.clear();
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        return true;
    }
    for (const auto& it : std::filesystem::directory_iterator{dir}) {
        if (!it.is_regular_file()) {
            continue;
        }
        if (it.path().extension() == ".meta") {
            metas.push_back(it.path());
        }
    }
    std::sort(metas.begin(), metas.end());
    return true;
}

bool storage_list_album_names(const std::filesystem::path& albums_root, std::vector<std::string>& names) {
    names.clear();
    if (!std::filesystem::exists(albums_root) || !std::filesystem::is_directory(albums_root)) {
        return true;
    }
    for (const auto& it : std::filesystem::directory_iterator{albums_root}) {
        if (!it.is_directory()) {
            continue;
        }
        names.push_back(it.path().filename().string());
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    return true;
}

std::filesystem::path storage_bin_from_meta(const std::filesystem::path& meta) {
    std::filesystem::path b{meta};
    b.replace_extension(".bin");
    return b;
}

static bool move_one(const std::filesystem::path& src, const std::filesystem::path& dst, std::string& err) {
    std::error_code ec{};
    std::filesystem::rename(src, dst, ec);
    if (!ec) {
        return true;
    }
    std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        err = "move failed";
        return false;
    }
    std::filesystem::remove(src, ec);
    if (ec) {
        err = "cleanup failed";
        return false;
    }
    return true;
}

bool storage_move_pair(const std::filesystem::path& bin_src, const std::filesystem::path& meta_src,
                       const std::filesystem::path& dst_dir, std::string& err) {
    auto bin_dst = dst_dir / bin_src.filename();
    auto meta_dst = dst_dir / meta_src.filename();
    if (!move_one(bin_src, bin_dst, err)) {
        return false;
    }
    if (!move_one(meta_src, meta_dst, err)) {
        return false;
    }
    return true;
}

std::mutex& storage_mutex() {
    return g_storage_mutex;
}
