cmake_minimum_required(VERSION 3.22)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(myftp-lab)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE "Release")
add_compile_options("-Wall")

set(SERVER_SOURCES
    src/ftp_server.cxx
    src/error_handle.cxx
    src/file_process.cxx
    src/socket.cxx
    src/tools.cxx
)

set(CLIENT_SOURCES
    src/ftp_client.cxx
    src/error_handle.cxx
    src/file_process.cxx
    src/socket.cxx
    src/tools.cxx
)

add_executable(ftp_server ${SERVER_SOURCES})
add_executable(ftp_client ${CLIENT_SOURCES})
