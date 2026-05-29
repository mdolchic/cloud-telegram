#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

struct HttpRequest {
    std::string method{};
    std::string target{};
    std::string path{};
    std::map<std::string, std::string> query{};
    std::map<std::string, std::string> headers{};
    std::vector<std::uint8_t> body{};
};

struct HttpResponse {
    int status{200};
    std::string content_type{"application/json; charset=utf-8"};
    std::vector<std::pair<std::string, std::string>> headers{};
    std::string body{};
};
