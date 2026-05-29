#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct AlbumItem {
    std::string id{};
    char kind{};
    std::string name{};
    std::filesystem::path bin_path{};
    std::size_t size_bytes{};
};

bool service_add_file(const std::string& uid, char kind, const std::string& name,
                      const std::vector<std::uint8_t>& data, std::string& id, std::string& err);
bool service_commit_album(const std::string& uid, const std::string& album, std::size_t& moved, std::string& err);
bool service_list_albums(const std::string& uid, std::vector<std::string>& names, std::string& err);
bool service_get_album(const std::string& uid, const std::string& album, std::vector<AlbumItem>& items, std::string& err);
bool service_get_album_item(const std::string& uid, const std::string& album, const std::string& item_id,
                            AlbumItem& item, std::string& err);
