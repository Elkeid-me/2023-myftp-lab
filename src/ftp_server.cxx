#include "file_process.hxx"
#include "socket.hxx"
#include "tools.hxx"
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>

bool check_ip(const char *ip, const char *port);
void ftp_server_open_connection_function(int fd_to_client);
void ftp_server_main_process_function(int fd_to_client);

[[nodiscard]] bool quit_connection(int fd_to_client);
[[nodiscard]] bool open_connection(int fd_to_client);
[[nodiscard]] bool list(int fd_to_client, char *buf);
[[nodiscard]] bool sha256(int fd_to_client, char *buf,
                          std::uint32_t file_name_length);
[[nodiscard]] bool upload_file(int fd_to_client, char *buf,
                               std::uint32_t file_name_length);
[[nodiscard]] bool download_file(int fd_to_client, char *buf,
                                 std::uint32_t file_name_length);

int main(int argc, char *argv[])
{
    std::signal(SIGPIPE, SIG_IGN);

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

    int listen_fd{socket_process::open_listen_fd(ip, port)};

    if (listen_fd < 0)
        return 1;

    while (true)
    {
        socklen_t client_len{sizeof(sockaddr_storage)};
        sockaddr_storage client_addr;
        int fd_to_client{socket_process::accept(
            listen_fd, reinterpret_cast<sockaddr *>(&client_addr),
            &client_len)};

        std::thread new_thread(ftp_server_open_connection_function,
                               fd_to_client);
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

void ftp_server_open_connection_function(int fd_to_client)
{
    myftp_head myftp_head_buf;

    if (!myftp_head_buf.get(fd_to_client))
    {
        file_process::close(fd_to_client);
        return;
    }
    if (myftp_head_buf.get_type() != MYFTP_HEAD_TYPE::OPEN_CONNECTION_REQUEST)
    {
        file_process::close(fd_to_client);
        return;
    }
    if (!open_connection(fd_to_client))
    {
        file_process::close(fd_to_client);
        return;
    }

    ftp_server_main_process_function(fd_to_client);
    file_process::close(fd_to_client);
}

void ftp_server_main_process_function(int fd_to_client)
{
    myftp_head myftp_head_buf;
    char file_buf[BUF_SIZE];

    bool is_connected{false};

    while (true)
    {
        if (!myftp_head_buf.get(fd_to_client))
            return;

        switch (myftp_head_buf.get_type())
        {
        case MYFTP_HEAD_TYPE::LIST_REQUEST:
            is_connected = list(fd_to_client, file_buf);
            break;
        case MYFTP_HEAD_TYPE::GET_REQUEST:
            is_connected = download_file(fd_to_client, file_buf,
                                         myftp_head_buf.get_payload_length());
            break;
        case MYFTP_HEAD_TYPE::PUT_REQUEST:
            is_connected = upload_file(fd_to_client, file_buf,
                                       myftp_head_buf.get_payload_length());
            break;
        case MYFTP_HEAD_TYPE::SHA_REQUEST:
            is_connected = sha256(fd_to_client, file_buf,
                                  myftp_head_buf.get_payload_length());
            break;
        case MYFTP_HEAD_TYPE::QUIT_REQUEST:
            is_connected = quit_connection(fd_to_client);
            break;
        default:
            is_connected = false;
        }

        if (!is_connected)
            return;
    }
}

[[nodiscard]] bool open_connection(int fd_to_client)
{
    if (!OPEN_CONNECTION_REPLY.send(fd_to_client))
        return false;
    return true;
}

[[nodiscard]] bool upload_file(int fd_to_client, char *buf,
                               std::uint32_t file_name_length)
{
    if (file_process::read(fd_to_client, buf, file_name_length) !=
        file_name_length)
        return false;
    std::string_view path{buf, file_name_length - 1};

    if (!PUT_REPLY.send(fd_to_client))
        return false;

    myftp_head tmp_head;
    if (!tmp_head.get(fd_to_client) ||
        tmp_head.get_type() != MYFTP_HEAD_TYPE::FILE_DATA)
        return false;

    std::size_t file_size{tmp_head.get_payload_length()};

    if (!receive_file(fd_to_client, path.data(), buf, file_size))
        return false;

    return true;
}

[[nodiscard]] bool download_file(int fd_to_client, char *buf,
                                 std::uint32_t file_name_length)
{
    if (file_process::read(fd_to_client, buf, file_name_length) !=
        file_name_length)
        return false;
    std::string_view path{buf, file_name_length - 1};
    if (!std::filesystem::is_regular_file(path))
    {
        if (!GET_REPLY_FAIL.send(fd_to_client))
            return false;
    }
    else
    {
        if (!GET_REPLY_SUCCESS.send(fd_to_client))
            return false;

        std::size_t file_size{std::filesystem::file_size(path)};

        myftp_head get_reply(MYFTP_HEAD_TYPE::FILE_DATA, 1,
                             MYFTP_HEAD_SIZE + file_size);

        if (!get_reply.send(fd_to_client))
            return false;

        if (!send_file(fd_to_client, path.data(), buf, file_size))
            return false;
    }
    return true;
}

[[nodiscard]] bool quit_connection(int fd_to_client)
{
    bool  make_gcc_happy [[maybe_unused]] {QUIT_REPLY.send(fd_to_client)};
    return false;
}

[[nodiscard]] bool list(int fd_to_client, char *buf)
{
    FILE_ptr read_fp{popen("ls", "r")};
    if (read_fp.is_valid())
    {
        if (std::size_t read_num{
                fread(buf, sizeof(char), BUF_SIZE - 1, read_fp.get_ptr())};
            read_num > 0)
        {
            buf[read_num] = 0;

            myftp_head list_reply(MYFTP_HEAD_TYPE::LIST_REPLY, 1,
                                  MYFTP_HEAD_SIZE + read_num + 1);

            if (!list_reply.send(fd_to_client))
                return false;
            if (file_process::write(fd_to_client, buf, read_num + 1) !=
                read_num + 1)
                return false;
        }
    }
    return true;
}

[[nodiscard]] bool sha256(int fd_to_client, char *buf,
                          std::uint32_t file_name_length)
{
    if (file_process::read(fd_to_client, buf, file_name_length) !=
        file_name_length)
        return false;

    std::string cmd{"sha256sum "};
    cmd += buf;

    std::string path{"./"};
    path += buf;

    if (!std::filesystem::is_regular_file(path))
    {
        if (!SHA_REPLAY_FAIL.send(fd_to_client))
            return false;
    }
    else
    {
        FILE_ptr read_fp{popen(cmd.c_str(), "r")};
        if (read_fp.is_valid())
        {
            if (std::size_t read_num{
                    fread(buf, sizeof(char), BUF_SIZE - 1, read_fp.get_ptr())};
                read_num > 0)
            {
                buf[read_num] = 0;

                if (!SHA_REPLAY_SUCCESS.send(fd_to_client))
                    return false;

                myftp_head sha_reply_head(MYFTP_HEAD_TYPE::FILE_DATA, 1,
                                          MYFTP_HEAD_SIZE + read_num + 1);

                if (!sha_reply_head.send(fd_to_client))
                    return false;

                if (file_process::write(fd_to_client, buf, read_num + 1) !=
                    read_num + 1)
                    return false;
            }
        }
    }
    return true;
}
