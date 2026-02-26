//
// Created by Максим Долганов on 7.01.26.
//

#pragma once
#include <filesystem>
#include <string>

bool item_send(int fd, char kind, const std::string& name, const std::filesystem::path& bin_path);