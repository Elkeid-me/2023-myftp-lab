#include "file_process.hxx"
#include "socket.hxx"
#include "tools.hxx"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>

enum class COMMAND_TYPE
{
    OPEN,
    LIST,
    GET,
    PUT,
    SHA,
    QUIT,
    INVALID
};

const std::regex LIST_COMMAND_PATTERN{R"(ls\s*)"};
const std::regex OPEN_COMMAND_PATTERN{R"(open (\s+) (\s+)\s)"};
const std::regex SHA_COMMAND_PATTERN{R"(sha256 (.+))"};

std::tuple<COMMAND_TYPE, std::string_view, std::string_view>
parse_command(std::string_view command);

int open_connection(const char *ip, const char *port);

bool list(int fd_to_server, char *buf);
bool quit(int fd_to_server);
bool sha256(int fd_to_server, std::string_view file_name, char *buf);

int main()
{
    std::cerr << "hello from ftp client" << std::endl;
    return 0;
}

constexpr std::string_view PROMPT{"client"};
void ftp_client_loop()
{
    char buf[BUF_SIZE];

    std::string command;
    bool is_connected{false};
    int fd_to_server;
    std::string server_ip, server_port;
    while (true)
    {
        /* print prompt */
        std::cout << PROMPT;
        if (is_connected)
            std::cout << '(' << server_ip << ':' << server_port << ") >";
        else
            std::cout << "(none) >";
        /* prompt printed */

        std::getline(std::cin, command);
        auto [command_type, str_1, str_2]{parse_command(command)};

        switch (command_type)
        {
        case COMMAND_TYPE::OPEN:
            server_ip = str_1;
            server_port = str_2;
            fd_to_server =
                open_connection(server_ip.c_str(), server_port.c_str());
            if (fd_to_server >= 0)
                is_connected = true;
            break;
        case COMMAND_TYPE::LIST:
            is_connected = list(fd_to_server, buf);
            break;
        case COMMAND_TYPE::GET:
        case COMMAND_TYPE::PUT:
        case COMMAND_TYPE::SHA:
            is_connected = sha256(fd_to_server, str_1, buf);
            break;
        case COMMAND_TYPE::QUIT:
            is_connected = quit(fd_to_server);
            break;
        case COMMAND_TYPE::INVALID:
            std::cout << "Invalid command.";
            break;
        }
    }
}

int open_connection(const char *ip, const char *port)
{
    myftp_head head_buf;

    int fd_to_server = socket_process::open_client_fd(ip, port);
    if (fd_to_server < 0)
    {
        std::cout << "Connect to server: " << ip << ':' << port << " error.\n";
        return -1;
    }

    OPEN_CONNECTION_REQUEST.send(fd_to_server);
    head_buf.get(fd_to_server);
    if (head_buf.get_type() != MYFTP_HEAD_TYPE::OPEN_CONNECTION_REPLY)
    {
        file_process::close(fd_to_server);
        std::cout << "Connect to server: " << ip << ':' << port << " error.\n";
        return -1;
    }

    return fd_to_server;
}

bool sha256(int fd_to_server, std::string_view file_name, char *buf)
{
    myftp_head head_buf(MYFTP_HEAD_TYPE::SHA_REQUEST, 1,
                        MYFTP_HEAD_SIZE + file_name.length() + 1);
    head_buf.send(fd_to_server);
    file_process::write(fd_to_server, file_name.data(), file_name.length());
    file_process::write(fd_to_server, "\0", 1);

    head_buf.get(fd_to_server);

    if (head_buf.get_type() != MYFTP_HEAD_TYPE::SHA_REPLY)
    {
        file_process::close(fd_to_server);
        std::cout << "Sha256sum file error.\n";
        return false;
    }

    switch (head_buf.get_status())
    {
    case 0:
        std::cout << "File not exists.\n";
        break;
    case 1:
        head_buf.get(fd_to_server);
        if (head_buf.get_type() != MYFTP_HEAD_TYPE::FILE_DATA)
        {
            file_process::close(fd_to_server);
            std::cout << "Sha256sum file error.\n";
            return false;
        }

        file_process::read(fd_to_server, buf, head_buf.get_payload_length());

        std::cout << "------Sha256 result------\n";
        std::cout << buf;
        std::cout << "----Sha256 result end----\n";
    }
    return true;
}

bool list(int fd_to_server, char *buf)
{

    myftp_head head_buf;
    LIST_REQUEST.send(fd_to_server);

    head_buf.get(fd_to_server);
    if (head_buf.get_type() != MYFTP_HEAD_TYPE::LIST_REPLY)
    {
        file_process::close(fd_to_server);
        std::cout << "List file error.\n";
        return false;
    }

    file_process::read(fd_to_server, buf, head_buf.get_payload_length());

    std::cout << "------List of files------\n";
    std::cout << buf;
    std::cout << "----List of files end----\n";

    return true;
}

bool quit(int fd_to_server)
{
    myftp_head head_buf;

    QUIT_REQUEST.send(fd_to_server);
    head_buf.get(fd_to_server);

    if (head_buf.get_type() == MYFTP_HEAD_TYPE::QUIT_REPLY)
        std::cout << "Quit successfully.\n";
    else
        std::cout << "Quit connection error.\n";

    file_process::close(fd_to_server);
    return false;
}

std::tuple<COMMAND_TYPE, std::string_view, std::string_view>
parse_command(std::string_view command)
{
    std::cmatch m;
    if (std::regex_match(command, SHA_COMMAND_PATTERN, m))
}
