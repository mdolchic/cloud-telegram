cmake_minimum_required(VERSION 3.16)
project(database)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

add_library(server_core
        src/domain/storage.cpp
        src/domain/meta.cpp
        src/domain/id_gen.cpp
        src/domain/service.cpp
        src/http/http_parser.cpp
        src/http/http_response.cpp
        src/http/http_router.cpp
        src/app/http_session.cpp
        src/infra/thread_pool.cpp
)

target_include_directories(server_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(database_server
        src/app/server.cpp
)

target_link_libraries(database_server PRIVATE server_core)
