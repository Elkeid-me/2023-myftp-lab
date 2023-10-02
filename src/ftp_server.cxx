#include "file_process.hxx"
#include "socket.hxx"
#include "tools.hxx"
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <tuple>

constexpr std::string_view MYFTP_HEADER{"\xc1\xa1\x10"
                                        "ftp"};

bool check_ip(const char *ip, const char *port);
void ftp_server_thread_function(int fd_to_client);

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

const std::regex PORT_PATTERN{"[0-9]"};
const std::regex IPv4_PATTERN{R"([0-9]{3}(\.[0-9]{3}){3})"};
const std::regex IPv6_PATTERN{R"(([0-9]|[a-f])(:([0-9]|[a-f])){7})"};

bool check_ip(const char *ip, const char *port)
{
    if (!std::regex_match(ip, IPv4_PATTERN) && !std::regex_match(ip, IPv6_PATTERN))
        return false;
    if (!std::regex_match(port, PORT_PATTERN))
        return false;
    return true;
}

void ftp_server_thread_function(int fd_to_client)
{
    char buf[102400];
    Read(fd_to_client, buf, 12);
}
