#include "app/http_session.h"

#include "http/http_parser.h"
#include "http/http_response.h"
#include "http/http_router.h"

#include <iostream>
#include <string>

void http_handle_client(ClientSocket client) {
    HttpRequest request{};
    std::string err{};
    if (!http_read_request(client.get(), request, err)) {
        const auto response = http_make_json_error(400, err);
        if (!http_write_response(client.get(), response)) {
            std::cerr << "failed to write bad request response\n";
        }
        return;
    }

    const auto response = http_route_request(request);
    if (!http_write_response(client.get(), response)) {
        std::cerr << "failed to write http response\n";
    }
}
