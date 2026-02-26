//
// Created by Максим Долганов on 7.01.26.
//

#pragma once
#include <filesystem>
#include <string>
#include <vector>

bool storage_user_inbox(const std::string& uid, std::filesystem::path& out, std::string& err);
bool storage_user_albums_root(const std::string& uid, std::filesystem::path& out, std::string& err);
bool storage_album_dir(const std::string& uid, const std::string& album, std::filesystem::path& out, std::string& err);

bool storage_list_meta_files(const std::filesystem::path& dir, std::vector<std::filesystem::path>& metas);
bool storage_list_album_names(const std::filesystem::path& albums_root, std::vector<std::string>& names);

std::filesystem::path storage_bin_from_meta(const std::filesystem::path& meta);
bool storage_move_pair(const std::filesystem::path& bin_src, const std::filesystem::path& meta_src,
                       const std::filesystem::path& dst_dir, std::string& err);