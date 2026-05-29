#include "domain/service.h"

#include "domain/id_gen.h"
#include "domain/meta.h"
#include "domain/storage.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
    constexpr std::size_t kMaxFileBytes{25ull * 1024ull * 1024ull};

    bool is_valid_kind(char kind) {
        return kind == 'P' || kind == 'D';
    }

    bool is_valid_item_id(const std::string& item_id) {
        if (item_id.empty()) {
            return false;
        }
        if (item_id.find("..") != std::string::npos) {
            return false;
        }
        return item_id.find_first_of("/\\") == std::string::npos;
    }
}

bool service_add_file(const std::string& uid, char kind, const std::string& name,
                      const std::vector<std::uint8_t>& data, std::string& id, std::string& err) {
    if (!is_valid_kind(kind)) {
        err = "bad kind";
        return false;
    }
    if (name.size() > 4096) {
        err = "name too long";
        return false;
    }
    if (data.empty() || data.size() > kMaxFileBytes) {
        err = "bad file size";
        return false;
    }

    std::lock_guard lock{storage_mutex()};

    std::filesystem::path inbox{};
    if (!storage_user_inbox(uid, inbox, err)) {
        return false;
    }

    id = id_gen_make();
    auto bin_path = inbox / (id + ".bin");
    auto meta_path = inbox / (id + ".meta");

    {
        std::ofstream out{bin_path, std::ios::binary};
        if (!out) {
            err = "failed to create file";
            return false;
        }
        out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        if (!out) {
            err = "failed to write file";
            return false;
        }
        out.flush();
        if (!out) {
            err = "failed to flush file";
            return false;
        }
    }

    if (!meta_write(meta_path, kind, name)) {
        std::error_code ec{};
        std::filesystem::remove(bin_path, ec);
        err = "failed to write meta";
        return false;
    }

    return true;
}

bool service_commit_album(const std::string& uid, const std::string& album, std::size_t& moved, std::string& err) {
    moved = 0;
    std::lock_guard lock{storage_mutex()};

    std::filesystem::path inbox{};
    if (!storage_user_inbox(uid, inbox, err)) {
        return false;
    }

    std::filesystem::path album_dir{};
    if (!storage_album_dir(uid, album, album_dir, err)) {
        return false;
    }

    std::vector<std::filesystem::path> metas{};
    storage_list_meta_files(inbox, metas);

    for (const auto& meta_src : metas) {
        auto bin_src = storage_bin_from_meta(meta_src);
        if (!std::filesystem::exists(bin_src)) {
            err = "corrupt inbox";
            return false;
        }
        if (!storage_move_pair(bin_src, meta_src, album_dir, err)) {
            return false;
        }
        moved += 1;
    }

    return true;
}

bool service_list_albums(const std::string& uid, std::vector<std::string>& names, std::string& err) {
    std::lock_guard lock{storage_mutex()};

    std::filesystem::path root{};
    if (!storage_user_albums_root(uid, root, err)) {
        return false;
    }

    storage_list_album_names(root, names);
    return true;
}

bool service_get_album(const std::string& uid, const std::string& album, std::vector<AlbumItem>& items, std::string& err) {
    items.clear();
    std::lock_guard lock{storage_mutex()};

    std::filesystem::path album_dir{};
    if (!storage_album_dir(uid, album, album_dir, err)) {
        return false;
    }

    std::vector<std::filesystem::path> metas{};
    storage_list_meta_files(album_dir, metas);

    items.reserve(metas.size());
    for (const auto& meta_path : metas) {
        auto bin_path = storage_bin_from_meta(meta_path);
        if (!std::filesystem::exists(bin_path)) {
            err = "corrupt album";
            return false;
        }

        char kind{};
        std::string name{};
        if (!meta_read(meta_path, kind, name)) {
            err = "corrupt meta";
            return false;
        }

        std::error_code ec{};
        const auto size = std::filesystem::file_size(bin_path, ec);
        if (ec) {
            err = "size error";
            return false;
        }

        items.push_back(AlbumItem{
            .id = meta_path.stem().string(),
            .kind = kind,
            .name = std::move(name),
            .bin_path = std::move(bin_path),
            .size_bytes = static_cast<std::size_t>(size),
        });
    }

    return true;
}

bool service_get_album_item(const std::string& uid, const std::string& album, const std::string& item_id,
                            AlbumItem& item, std::string& err) {
    if (!is_valid_item_id(item_id)) {
        err = "bad item id";
        return false;
    }

    std::lock_guard lock{storage_mutex()};

    std::filesystem::path album_dir{};
    if (!storage_album_dir(uid, album, album_dir, err)) {
        return false;
    }

    const auto meta_path = album_dir / (item_id + ".meta");
    const auto bin_path = album_dir / (item_id + ".bin");
    if (!std::filesystem::exists(meta_path) || !std::filesystem::exists(bin_path)) {
        err = "item not found";
        return false;
    }

    char kind{};
    std::string name{};
    if (!meta_read(meta_path, kind, name)) {
        err = "corrupt meta";
        return false;
    }

    std::error_code ec{};
    const auto size = std::filesystem::file_size(bin_path, ec);
    if (ec) {
        err = "size error";
        return false;
    }

    item = AlbumItem{
        .id = item_id,
        .kind = kind,
        .name = std::move(name),
        .bin_path = std::move(bin_path),
        .size_bytes = static_cast<std::size_t>(size),
    };
    return true;
}
