//
// Created by Максим Долганов on 7.01.26.
//

#pragma once
#include <string>

bool cmd_addf(int fd, const std::string& uid, char kind, std::size_t name_len, std::size_t nbytes);
bool cmd_commit(int fd, const std::string& uid, const std::string& album);
bool cmd_listalbums(int fd, const std::string& uid);
bool cmd_getalbum(int fd, const std::string& uid, const std::string& album);