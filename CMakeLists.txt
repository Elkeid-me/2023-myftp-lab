project(2023-lab1-myFTP)
cmake_minimum_required(VERSION 3.22)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

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
