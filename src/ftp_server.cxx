#include "file_process.hxx"
#include "socket.hxx"
#include "tools.hxx"
#include <filesystem>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>

bool check_ip(const char *ip, const char *port);
void ftp_server_thread_function(int fd_to_client);
void open_connection(int fd_to_client);
void quit_connection(int fd_to_client);
void list(int fd_to_client);

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <ip> <port>" << std::endl;
        return 1;
    }

    const char *ip{argv[1]}, *port{argv[2]};

    if (!check_ip(ip, port))
    {
        std::cerr << "Usage: " << argv[0] << " <ip> <port>" << std::endl;
        return 1;
    }

    int listen_fd{Open_listen_fd(port)};

    if (listen_fd < 0)
        return 1;

    while (true)
    {
        socklen_t client_len{sizeof(sockaddr_storage)};
        sockaddr_storage client_addr;
        int fd_to_client{Accept(listen_fd,
                                reinterpret_cast<sockaddr *>(&client_addr),
                                &client_len)};

        std::thread new_thread(ftp_server_thread_function, fd_to_client);
        new_thread.detach();
    }

    return 0;
}

bool check_ip(const char *ip, const char *port)
{
    if (!std::regex_match(ip, IPv4_PATTERN) &&
        !std::regex_match(ip, IPv6_PATTERN))
        return false;
    if (!std::regex_match(port, PORT_PATTERN))
        return false;
    return true;
}

void ftp_server_thread_function(int fd_to_client)
{
    myftp_head myftp_head_buf;
    char file_buf[102400];

    while (true)
    {
        myftp_head_buf.get(fd_to_client);

        if (!myftp_head_buf.is_valid())
        {
            quit_connection(fd_to_client);
            return;
        }

        switch (myftp_head_buf.get_type())
        {
        case MYFTP_HEAD_TYPE::OPEN_CONNECTION_REQUEST:
            open_connection(fd_to_client);
            break;
        case MYFTP_HEAD_TYPE::LIST_REQUEST:
            list(fd_to_client);
            break;
        case MYFTP_HEAD_TYPE::GET_REQUEST:
        case MYFTP_HEAD_TYPE::PUT_REQUEST:

        case MYFTP_HEAD_TYPE::SHA_REQUEST:
            break;
        case MYFTP_HEAD_TYPE::QUIT_REQUEST:
        default:
            quit_connection(fd_to_client);
            return;
        }
    }
}

void open_connection(int fd_to_client)
{
    OPEN_CONNECTION_REPLY.send(fd_to_client);
}

void quit_connection(int fd_to_client)
{
    QUIT_REPLY.send(fd_to_client);
    Close(fd_to_client);
}

void list(int fd_to_client)
{
    std::string ret;
    for (auto &p : std::filesystem::directory_iterator("."))
    {
        std::string file_name{p.path().filename().generic_string()};
        if (!file_name.starts_with('.'))
            ret += file_name + '\n';
    }
    myftp_head list_reply(MYFTP_HEAD_TYPE::LIST_REPLY, 1,
                          12 + ret.length() + 1);
    list_reply.send(fd_to_client);
    Write(fd_to_client, ret.c_str(), ret.length() + 1);
}
