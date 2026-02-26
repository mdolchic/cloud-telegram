//
// Created by Максим Долганов on 7.01.26.
//

#pragma once
#include <filesystem>
#include <string>

bool meta_write(const std::filesystem::path& meta_path, char kind, const std::string& name);
bool meta_read(const std::filesystem::path& meta_path, char& kind, std::string& name);