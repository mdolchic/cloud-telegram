#include "http/http_router.h"

#include "domain/service.h"
#include "http/http_response.h"

#include <cstddef>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {
    bool from_hex(char c, unsigned char& value) {
        if (c >= '0' && c <= '9') {
            value = static_cast<unsigned char>(c - '0');
            return true;
        }
        if (c >= 'a' && c <= 'f') {
            value = static_cast<unsigned char>(10 + c - 'a');
            return true;
        }
        if (c >= 'A' && c <= 'F') {
            value = static_cast<unsigned char>(10 + c - 'A');
            return true;
        }
        return false;
    }

    bool percent_decode(const std::string& input, std::string& output) {
        output.clear();
        output.reserve(input.size());
        for (std::size_t i = 0; i < input.size(); ++i) {
            const char c = input[i];
            if (c == '%') {
                if (i + 2 >= input.size()) {
                    return false;
                }
                unsigned char hi{};
                unsigned char lo{};
                if (!from_hex(input[i + 1], hi) || !from_hex(input[i + 2], lo)) {
                    return false;
                }
                output.push_back(static_cast<char>((hi << 4u) | lo));
                i += 2;
                continue;
            }
            if (c == '+') {
                output.push_back(' ');
                continue;
            }
            output.push_back(c);
        }
        return true;
    }

    std::vector<std::string> split_path(const std::string& path, bool& ok) {
        ok = true;
        std::vector<std::string> parts{};
        std::stringstream ss{path};
        std::string segment{};
        while (std::getline(ss, segment, '/')) {
            if (segment.empty()) {
                continue;
            }
            std::string decoded{};
            if (!percent_decode(segment, decoded)) {
                ok = false;
                return {};
            }
            parts.push_back(std::move(decoded));
        }
        return parts;
    }

    std::string json_albums(const std::vector<std::string>& names) {
        std::string body = "{\"albums\":[";
        for (std::size_t i = 0; i < names.size(); ++i) {
            if (i != 0) {
                body += ",";
            }
            body += "\"" + http_json_escape(names[i]) + "\"";
        }
        body += "]}";
        return body;
    }

    std::string json_album_items(const std::vector<AlbumItem>& items) {
        std::string body = "{\"items\":[";
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (i != 0) {
                body += ",";
            }
            body += "{\"id\":\"" + http_json_escape(items[i].id) + "\",";
            body += "\"kind\":\"" + std::string(1, items[i].kind) + "\",";
            body += "\"name\":\"" + http_json_escape(items[i].name) + "\",";
            body += "\"size_bytes\":" + std::to_string(items[i].size_bytes) + "}";
        }
        body += "]}";
        return body;
    }
}

HttpResponse http_route_request(const HttpRequest& request) {
    bool path_ok{};
    const auto path = split_path(request.path, path_ok);
    if (!path_ok) {
        return http_make_json_error(400, "bad path");
    }

    if (request.method == "GET" && path.size() == 1 && path[0] == "health") {
        return http_make_json_response(200, "{\"status\":\"ok\"}");
    }

    if (path.size() < 3 || path[0] != "users") {
        return http_make_json_error(404, "route not found");
    }

    const auto& uid = path[1];

    if (request.method == "POST" && path.size() == 3 && path[2] == "files") {
        const auto kind_it = request.query.find("kind");
        const auto name_it = request.query.find("name");
        if (kind_it == request.query.end() || name_it == request.query.end() || kind_it->second.size() != 1) {
            return http_make_json_error(400, "kind and name query params are required");
        }

        std::string id{};
        std::string err{};
        if (!service_add_file(uid, kind_it->second[0], name_it->second, request.body, id, err)) {
            return http_make_json_error(400, err);
        }
        return http_make_json_response(201, "{\"id\":\"" + http_json_escape(id) + "\"}");
    }

    if (request.method == "GET" && path.size() == 3 && path[2] == "albums") {
        std::vector<std::string> names{};
        std::string err{};
        if (!service_list_albums(uid, names, err)) {
            return http_make_json_error(400, err);
        }
        return http_make_json_response(200, json_albums(names));
    }

    if (request.method == "POST" && path.size() == 5 && path[2] == "albums" && path[4] == "commit") {
        std::size_t moved{};
        std::string err{};
        if (!service_commit_album(uid, path[3], moved, err)) {
            return http_make_json_error(400, err);
        }
        return http_make_json_response(200, "{\"moved\":" + std::to_string(moved) + "}");
    }

    if (request.method == "GET" && path.size() == 4 && path[2] == "albums") {
        std::vector<AlbumItem> items{};
        std::string err{};
        if (!service_get_album(uid, path[3], items, err)) {
            return http_make_json_error(400, err);
        }
        return http_make_json_response(200, json_album_items(items));
    }

    if (request.method == "GET" && path.size() == 7 && path[2] == "albums" && path[4] == "items" && path[6] == "content") {
        AlbumItem item{};
        std::string err{};
        if (!service_get_album_item(uid, path[3], path[5], item, err)) {
            return http_make_json_error(err == "item not found" ? 404 : 400, err);
        }
        return http_make_file_response(item);
    }

    return http_make_json_error(404, "route not found");
}
