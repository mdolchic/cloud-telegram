cmake_minimum_required(VERSION 3.16)
project(database)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

add_library(server_core
        src/protocol.cpp
        src/cmd_addf.cpp
        src/cmd_commit.cpp
        src/cmd_listalbums.cpp
        src/cmd_getalbum.cpp
        src/storage.cpp
        src/meta.cpp
        src/id_gen.cpp
        src/item_io.cpp
)

target_include_directories(server_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(database_server
        src/server.cpp
)

target_link_libraries(database_server PRIVATE server_core)