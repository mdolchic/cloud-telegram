#include "http/http_response.h"

#include "infra/net_io.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace {
    std::string reason_phrase(int status) {
        switch (status) {
            case 200:
                return "OK";
            case 201:
                return "Created";
            case 400:
                return "Bad Request";
            case 404:
                return "Not Found";
            case 405:
                return "Method Not Allowed";
            case 500:
                return "Internal Server Error";
            default:
                return "Error";
        }
    }
}

std::string http_json_escape(const std::string& input) {
    std::string out{};
    out.reserve(input.size() + 8);
    for (const char c : input) {
        switch (c) {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out.push_back(c);
                break;
        }
    }
    return out;
}

HttpResponse http_make_json_response(int status, const std::string& body) {
    return HttpResponse{
        .status = status,
        .content_type = "application/json; charset=utf-8",
        .headers = {},
        .body = body,
    };
}

HttpResponse http_make_json_error(int status, const std::string& message) {
    return http_make_json_response(status, "{\"error\":\"" + http_json_escape(message) + "\"}");
}

HttpResponse http_make_file_response(const AlbumItem& item) {
    return HttpResponse{
        .status = 200,
        .content_type = "application/octet-stream",
        .headers = {
            {"X-Item-Id", item.id},
            {"X-Item-Kind", std::string(1, item.kind)},
            {"X-Item-Name", item.name},
            {"X-File-Path", item.bin_path.string()},
        },
        .body = {},
    };
}

bool http_write_response(int fd, const HttpResponse& response) {
    std::string header = "HTTP/1.1 " + std::to_string(response.status) + " " + reason_phrase(response.status) + "\r\n";
    header += "Content-Type: " + response.content_type + "\r\n";

    const bool is_file_response = response.content_type == "application/octet-stream";
    std::string file_path{};
    for (const auto& [key, value] : response.headers) {
        if (key == "X-File-Path") {
            file_path = value;
            continue;
        }
        header += key + ": " + value + "\r\n";
    }

    if (!is_file_response) {
        header += "Content-Length: " + std::to_string(response.body.size()) + "\r\n";
        header += "Connection: close\r\n\r\n";
        return net_io::write_all(fd, header) &&
               net_io::write_exact(fd, response.body.data(), response.body.size());
    }

    std::ifstream in{file_path, std::ios::binary};
    if (!in) {
        return http_write_response(fd, http_make_json_error(500, "failed to open file"));
    }

    in.seekg(0, std::ios::end);
    const auto end = in.tellg();
    if (end < 0) {
        return http_write_response(fd, http_make_json_error(500, "failed to stat file"));
    }
    const auto size = static_cast<std::size_t>(end);
    in.seekg(0, std::ios::beg);

    header += "Content-Length: " + std::to_string(size) + "\r\n";
    header += "Connection: close\r\n\r\n";
    if (!net_io::write_all(fd, header)) {
        return false;
    }

    std::vector<std::uint8_t> buffer(64ull * 1024ull);
    while (in) {
        in.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        const auto read = in.gcount();
        if (read <= 0) {
            break;
        }
        if (!net_io::write_exact(fd, buffer.data(), static_cast<std::size_t>(read))) {
            return false;
        }
    }
    return static_cast<bool>(in.eof());
}
