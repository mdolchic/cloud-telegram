#pragma once

#include "domain/service.h"
#include "http/http_types.h"

#include <string>

std::string http_json_escape(const std::string& input);

HttpResponse http_make_json_response(int status, const std::string& body);
HttpResponse http_make_json_error(int status, const std::string& message);
HttpResponse http_make_file_response(const AlbumItem& item);

bool http_write_response(int fd, const HttpResponse& response);
