#include "http/http_parser.h"

#include "infra/net_io.h"

#include <cctype>
#include <cstddef>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {
    constexpr std::size_t kMaxBodyBytes{30ull * 1024ull * 1024ull};

    std::string trim(std::string value) {
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
            value.erase(value.begin());
        }
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
            value.pop_back();
        }
        return value;
    }

    std::string header_key(std::string value) {
        for (char& c : value) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return value;
    }

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

    std::map<std::string, std::string> parse_query(const std::string& query, bool& ok) {
        ok = true;
        std::map<std::string, std::string> values{};
        std::stringstream ss{query};
        std::string pair{};
        while (std::getline(ss, pair, '&')) {
            if (pair.empty()) {
                continue;
            }
            const auto pos = pair.find('=');
            const auto raw_key = pair.substr(0, pos);
            const auto raw_value = pos == std::string::npos ? std::string{} : pair.substr(pos + 1);
            std::string key{};
            std::string value{};
            if (!percent_decode(raw_key, key) || !percent_decode(raw_value, value)) {
                ok = false;
                return {};
            }
            values[key] = value;
        }
        return values;
    }

    bool parse_content_length(const std::map<std::string, std::string>& headers, std::size_t& content_length) {
        content_length = 0;
        const auto it = headers.find("content-length");
        if (it == headers.end()) {
            return true;
        }
        try {
            content_length = static_cast<std::size_t>(std::stoull(it->second));
        } catch (...) {
            return false;
        }
        return true;
    }
}

bool http_read_request(int fd, HttpRequest& request, std::string& err) {
    std::string line{};
    if (!net_io::read_line(fd, line)) {
        err = "failed to read request line";
        return false;
    }

    std::istringstream request_line{line};
    std::string version{};
    if (!(request_line >> request.method >> request.target >> version)) {
        err = "bad request line";
        return false;
    }
    if (version != "HTTP/1.1" && version != "HTTP/1.0") {
        err = "unsupported http version";
        return false;
    }

    request.headers.clear();
    while (true) {
        if (!net_io::read_line(fd, line)) {
            err = "failed to read headers";
            return false;
        }
        if (line.empty()) {
            break;
        }
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            err = "bad header";
            return false;
        }
        auto key = header_key(trim(line.substr(0, colon)));
        auto value = trim(line.substr(colon + 1));
        request.headers[std::move(key)] = std::move(value);
    }

    const auto query_pos = request.target.find('?');
    request.path = query_pos == std::string::npos ? request.target : request.target.substr(0, query_pos);
    const auto query_string = query_pos == std::string::npos ? std::string{} : request.target.substr(query_pos + 1);

    bool query_ok{};
    request.query = parse_query(query_string, query_ok);
    if (!query_ok) {
        err = "bad query";
        return false;
    }

    std::size_t content_length{};
    if (!parse_content_length(request.headers, content_length)) {
        err = "bad content-length";
        return false;
    }
    if (content_length > kMaxBodyBytes) {
        err = "request body too large";
        return false;
    }

    request.body.assign(content_length, 0);
    if (content_length != 0 && !net_io::read_exact(fd, request.body.data(), content_length)) {
        err = "failed to read body";
        return false;
    }

    return true;
}
