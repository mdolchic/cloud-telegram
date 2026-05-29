#pragma once

#include "http/http_types.h"

#include <string>

bool http_read_request(int fd, HttpRequest& request, std::string& err);
