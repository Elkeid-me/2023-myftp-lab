#include "file_process.hxx"
#include "socket.hxx"
#include "tools.hxx"
#include <csignal>
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
const std::regex OPEN_COMMAND_PATTERN{R"(open (\S+) (\S+)\s*)"};
const std::regex SHA_COMMAND_PATTERN{R"(sha256 (.+))"};
const std::regex QUIT_COMMAND_PATTERN{R"(quit\s*)"};
const std::regex GET_COMMAND_PATTERN{R"(get (.+))"};
const std::regex PUT_COMMAND_PATTERN{R"(put (.+))"};

void ftp_client_loop();
void connected_function(int fd_to_server, const std::string_view &ip,
                        const std::string_view &port);

std::tuple<COMMAND_TYPE, std::string_view, std::string_view>
parse_command(std::string_view command);

int open_connection(const char *ip, const char *port);

[[nodiscard]] bool list(int fd_to_server, char *buf);
[[nodiscard]] bool quit(int fd_to_server);
[[nodiscard]] bool sha256(int fd_to_server, std::string_view file_name,
                          char *buf);
[[nodiscard]] bool upload_file(int fd_to_server, std::string_view file_name,
                               char *buf);
[[nodiscard]] bool download_file(int fd_to_server, std::string_view file_name,
                                 char *buf);

int main()
{
    std::signal(SIGPIPE, SIG_IGN);
    ftp_client_loop();
    return 0;
}

constexpr std::string_view PROMPT{"client"};

void ftp_client_loop()
{
    char buf[BUF_SIZE];

    std::string command;
    int fd_to_server;
    std::string server_ip, server_port;
    while (true)
    {
        /* print prompt */
        std::cout << PROMPT << "(none) > ";
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
                connected_function(fd_to_server, str_1, str_2);
            break;
        case COMMAND_TYPE::QUIT:
            std::cout << "Quit myftp client.\n";
            return;
        default:
            std::cout << "Invalid command.\n";
            break;
        }
    }
}

void connected_function(int fd_to_server, const std::string_view &ip,
                        const std::string_view &port)
{
    char buf[BUF_SIZE];

    std::string command;

    bool is_connected{true};

    while (true)
    {
        /* print prompt */
        std::cout << PROMPT << '(' << ip << ':' << port << ") > ";
        /* prompt printed */
        std::getline(std::cin, command);
        auto [command_type, str_1, str_2]{parse_command(command)};

        switch (command_type)
        {
        case COMMAND_TYPE::LIST:
            is_connected = list(fd_to_server, buf);
            break;
        case COMMAND_TYPE::GET:
            is_connected = download_file(fd_to_server, str_1, buf);
            break;
        case COMMAND_TYPE::PUT:
            is_connected = upload_file(fd_to_server, str_1, buf);
            break;
        case COMMAND_TYPE::SHA:
            is_connected = sha256(fd_to_server, str_1, buf);
            break;
        case COMMAND_TYPE::QUIT:
            is_connected = quit(fd_to_server);
            return;
        default:
            std::cout << "Invalid command.\n";
            break;
        }

        if (!is_connected)
        {
            file_process::close(fd_to_server);
            return;
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

    if (!OPEN_CONNECTION_REQUEST.send(fd_to_server))
        goto open_connection_error;

    if (!head_buf.get(fd_to_server) ||
        head_buf.get_type() != MYFTP_HEAD_TYPE::OPEN_CONNECTION_REPLY)
        goto open_connection_error;

    return fd_to_server;

open_connection_error:
    file_process::close(fd_to_server);
    std::cout << "Connect to server: " << ip << ':' << port << " error.\n";
    return -1;
}

bool sha256(int fd_to_server, std::string_view file_name, char *buf)
{
    myftp_head head_buf(MYFTP_HEAD_TYPE::SHA_REQUEST, 1,
                        MYFTP_HEAD_SIZE + file_name.length() + 1);
    if (!head_buf.send(fd_to_server))
        goto sha256_error;
    if (file_process::write(fd_to_server, file_name.data(),
                            file_name.length()) != file_name.length() ||
        file_process::write(fd_to_server, "\0", 1) != 1)
        goto sha256_error;

    if (!head_buf.get(fd_to_server) ||
        head_buf.get_type() != MYFTP_HEAD_TYPE::SHA_REPLY)
        goto sha256_error;

    switch (head_buf.get_status())
    {
    case 0:
        std::cout << "Remote file `" << file_name << "' does not exist.\n";
        break;
    case 1:
        if (!head_buf.get(fd_to_server) ||
            head_buf.get_type() != MYFTP_HEAD_TYPE::FILE_DATA)
            goto sha256_error;

        if (file_process::read(fd_to_server, buf,
                               head_buf.get_payload_length()) !=
            head_buf.get_payload_length())
            goto sha256_error;

        std::cout << "------Sha256 result------\n";
        std::cout << buf;
        std::cout << "----Sha256 result end----\n";
    }
    return true;

sha256_error:
    file_process::close(fd_to_server);
    std::cout << "Sha256sum file error.\n";
    return false;
}

bool list(int fd_to_server, char *buf)
{

    myftp_head head_buf;
    if (!LIST_REQUEST.send(fd_to_server))
        goto list_error;

    if (!head_buf.get(fd_to_server) ||
        head_buf.get_type() != MYFTP_HEAD_TYPE::LIST_REPLY)
        goto list_error;

    if (file_process::read(fd_to_server, buf, head_buf.get_payload_length()) !=
        head_buf.get_payload_length())
        goto list_error;

    std::cout << "------List of files------\n";
    std::cout << buf;
    std::cout << "----List of files end----\n";

    return true;

list_error:
    file_process::close(fd_to_server);
    std::cout << "List file error.\n";
    return false;
}

bool quit(int fd_to_server)
{
    myftp_head head_buf;

    if (!QUIT_REQUEST.send(fd_to_server))
        goto quit_error;

    if (!head_buf.get(fd_to_server))
        goto quit_error;

    if (head_buf.get_type() == MYFTP_HEAD_TYPE::QUIT_REPLY)
        std::cout << "Quit successfully.\n";
    else
        std::cout << "Quit connection error.\n";

quit_error:
    file_process::close(fd_to_server);
    return false;
}

bool upload_file(int fd_to_server, std::string_view file_name, char *buf)
{
    std::string file_name_str(file_name);

    if (!std::filesystem::exists(file_name_str))
    {
        std::cout << "Local file `" << file_name_str << "' does not exist.\n";
        return true;
    }

    std::size_t file_size{std::filesystem::file_size(file_name_str)};
    // std::fstream fs(file_name_str, std::ios_base::in |
    // std::ios_base::binary);
    std::FILE *fs{fopen(file_name_str.c_str(), "rb")};

    myftp_head head_buf(MYFTP_HEAD_TYPE::PUT_REQUEST, 1,
                        MYFTP_HEAD_SIZE + file_name_str.length() + 1);
    if (!head_buf.send(fd_to_server))
        goto upload_file_error;

    if (file_process::write(fd_to_server, file_name_str.c_str(),
                            file_name_str.length() + 1) !=
        file_name_str.length() + 1)
        goto upload_file_error;

    if (!head_buf.get(fd_to_server) ||
        head_buf.get_type() != MYFTP_HEAD_TYPE::PUT_REPLY)
        goto upload_file_error;

    head_buf.pack(MYFTP_HEAD_TYPE::FILE_DATA, 1, MYFTP_HEAD_SIZE + file_size);

    if (!head_buf.send(fd_to_server))
        goto upload_file_error;

    while (true)
    {

        std::size_t read_num{fread(buf, sizeof(char), BUF_SIZE, fs)};
        // fs.read(buf, BUF_SIZE);
        // std::size_t read_num{static_cast<size_t>(fs.gcount())};
        if (file_process::write(fd_to_server, buf, read_num) != read_num)
        {
            std::cout << 114514;
            goto upload_file_error;
        }
        if (read_num < BUF_SIZE)
            break;
    }

    std::fclose(fs);
    return true;

upload_file_error:
    std::cout << "Upload file error.\n";
    file_process::close(fd_to_server);
    return false;
}

bool download_file(int fd_to_server, std::string_view file_name, char *buf)
{
    std::string file_name_str(file_name);

    myftp_head head_buf(MYFTP_HEAD_TYPE::GET_REQUEST, 1,
                        MYFTP_HEAD_SIZE + file_name_str.length() + 1);

    if (!head_buf.send(fd_to_server))
        goto download_file_error;

    if (file_process::write(fd_to_server, file_name_str.c_str(),
                            file_name_str.length() + 1) !=
        file_name_str.length() + 1)
        goto download_file_error;

    if (!head_buf.get(fd_to_server) ||
        head_buf.get_type() != MYFTP_HEAD_TYPE::GET_REPLY)
        goto download_file_error;

    switch (head_buf.get_status())
    {
    case 0:
        std::cout << "Remote file `" << file_name << "' does not exist.\n";
        break;
    case 1:
        if (!head_buf.get(fd_to_server) ||
            head_buf.get_type() != MYFTP_HEAD_TYPE::FILE_DATA)
            goto download_file_error;

        // std::fstream fs(file_name_str,
        //                 std::ios_base::out | std::ios_base::binary);

        std::FILE *fs{std::fopen(file_name_str.c_str(), "wb")};

        while (true)
        {
            ssize_t read_num{file_process::read(fd_to_server, buf, BUF_SIZE)};
            // fs.write(buf, read_num);
            std::fwrite(buf, sizeof(char), read_num, fs);
            if (read_num < BUF_SIZE)
                break;
        }

        std::fclose(fs);
    }

    return true;

download_file_error:
    std::cout << "Download file error.\n";
    file_process::close(fd_to_server);
    return false;
}

std::tuple<COMMAND_TYPE, std::string_view, std::string_view>
parse_command(std::string_view command)
{
    std::cmatch m;

    if (std::regex_match(command.begin(), command.end(), m,
                         OPEN_COMMAND_PATTERN))
    {
        if (!std::regex_match(m[1].first, m[1].second, IPv4_PATTERN) &&
                !std::regex_match(m[1].first, m[1].second, IPv6_PATTERN) ||
            !std::regex_match(m[2].first, m[2].second, PORT_PATTERN))
            return {COMMAND_TYPE::INVALID, {nullptr, 0}, {nullptr, 0}};
        return {COMMAND_TYPE::OPEN,
                {m[1].first, static_cast<std::size_t>(m[1].length())},
                {m[2].first, static_cast<std::size_t>(m[2].length())}};
    }

    if (std::regex_match(command.begin(), command.end(), m,
                         LIST_COMMAND_PATTERN))
        return {COMMAND_TYPE::LIST, {nullptr, 0}, {nullptr, 0}};

    if (std::regex_match(command.begin(), command.end(), m,
                         GET_COMMAND_PATTERN))
        return {COMMAND_TYPE::GET,
                {m[1].first, static_cast<std::size_t>(m[1].length())},
                {nullptr, 0}};

    if (std::regex_match(command.begin(), command.end(), m,
                         PUT_COMMAND_PATTERN))
        return {COMMAND_TYPE::PUT,
                {m[1].first, static_cast<std::size_t>(m[1].length())},
                {nullptr, 0}};

    if (std::regex_match(command.begin(), command.end(), m,
                         SHA_COMMAND_PATTERN))
        return {COMMAND_TYPE::SHA,
                {m[1].first, static_cast<std::size_t>(m[1].length())},
                {nullptr, 0}};

    if (std::regex_match(command.begin(), command.end(), m,
                         QUIT_COMMAND_PATTERN))
        return {COMMAND_TYPE::QUIT, {nullptr, 0}, {nullptr, 0}};

    return {COMMAND_TYPE::INVALID, {nullptr, 0}, {nullptr, 0}};
}
