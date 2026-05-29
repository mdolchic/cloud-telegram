//
// Created by Максим Долганов on 7.01.26.
//

#include "domain/meta.h"

#include <fstream>
#include <string>

bool meta_write(const std::filesystem::path& meta_path, char kind, const std::string& name) {
    std::ofstream out{meta_path, std::ios::binary};
    if (!out) {
        return false;
    }
    out << "KIND=" << kind << "\n";
    out << "NAME=" << name << "\n";
    out.flush();
    return static_cast<bool>(out);
}

bool meta_read(const std::filesystem::path& meta_path, char& kind, std::string& name) {
    std::ifstream in{meta_path, std::ios::binary};
    if (!in) { return false; }
    std::string l1{}, l2{};
    if (!std::getline(in, l1)) {
        return false;
    }
    if (!std::getline(in, l2)) {
        return false;
    }
    if (l1.rfind("KIND=", 0) != 0) {
        return false;
    }
    if (l2.rfind("NAME=", 0) != 0) {
        return false;
    }
    if (l1.size() < 6) {
        return false;
    }
    kind = l1[5];
    name = l2.substr(5);
    if (!(kind == 'P' || kind == 'D')) {
        return false;
    }
    return true;
}
