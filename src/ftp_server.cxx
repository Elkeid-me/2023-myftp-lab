#include "file_process.hxx"
#include "socket.hxx"
#include "tools.hxx"
#include <cstddef>
#include <cstdio>
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

std::fstream server_log("ftp_server.log");

bool check_ip(const char *ip, const char *port);
void ftp_server_thread_function(int fd_to_client);
void open_connection(int fd_to_client);
void quit_connection(int fd_to_client);
void list(int fd_to_client, char *buf);
void sha256(int fd_to_client, char *buf, std::uint32_t file_name_length);
void upload_file(int fd_to_client, char *buf, std::uint32_t file_name_length);
void download_file(int fd_to_client, char *buf, std::uint32_t file_name_length);

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

    server_log << "IP and Port Checked: " << ip << ":" << port << std::endl;

    int listen_fd{socket_process::open_listen_fd(port)};

    if (listen_fd < 0)
        return 1;

    while (true)
    {
        socklen_t client_len{sizeof(sockaddr_storage)};
        sockaddr_storage client_addr;
        int fd_to_client{socket_process::accept(
            listen_fd, reinterpret_cast<sockaddr *>(&client_addr),
            &client_len)};

        server_log << "Client Connected With File Description " << fd_to_client << std::endl;

        // std::thread new_thread(ftp_server_thread_function, fd_to_client);
        // new_thread.detach();

        ftp_server_thread_function(fd_to_client);
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
    char file_buf[BUF_SIZE];

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
            list(fd_to_client, file_buf);
            break;
        case MYFTP_HEAD_TYPE::GET_REQUEST:
            download_file(fd_to_client, file_buf,
                          myftp_head_buf.get_payload_length());
            break;
        case MYFTP_HEAD_TYPE::PUT_REQUEST:
            upload_file(fd_to_client, file_buf,
                        myftp_head_buf.get_payload_length());
            break;
        case MYFTP_HEAD_TYPE::SHA_REQUEST:
            sha256(fd_to_client, file_buf, myftp_head_buf.get_payload_length());
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

void upload_file(int fd_to_client, char *buf, std::uint32_t file_name_length)
{
    file_process::read(fd_to_client, buf, file_name_length);
    std::string_view path{buf, file_name_length - 1};

    PUT_REPLY.send(fd_to_client);

    myftp_head tmp_head;
    tmp_head.get(fd_to_client);

    std::size_t file_size{tmp_head.get_payload_length()};

    std::fstream fs(path.data(), std::ios_base::out | std::ios_base::binary);

    while (true)
    {
        std::size_t read_num{file_process::read(fd_to_client, buf, BUF_SIZE)};

        fs.write(buf, read_num);
        if (read_num < BUF_SIZE)
            break;
    }
}

void download_file(int fd_to_client, char *buf, std::uint32_t file_name_length)
{
    file_process::read(fd_to_client, buf, file_name_length);
    std::string_view path{buf, file_name_length - 1};
    if (!std::filesystem::exists(path))
        GET_REPLY_FAIL.send(fd_to_client);
    else
    {
        GET_REPLY_SUCCESS.send(fd_to_client);

        std::size_t file_size{std::filesystem::file_size(path)};

        std::fstream fs(path.data(), std::ios_base::in | std::ios_base::binary);

        myftp_head get_reply(MYFTP_HEAD_TYPE::FILE_DATA, 1,
                             MYFTP_HEAD_SIZE + file_size);
        get_reply.send(fd_to_client);

        while (true)
        {
            fs.read(buf, BUF_SIZE);
            std::size_t read_num{static_cast<size_t>(fs.gcount())};
            file_process::write(fd_to_client, buf, read_num);
            if (read_num < BUF_SIZE)
                break;
        }
    }
}

void quit_connection(int fd_to_client)
{
    QUIT_REPLY.send(fd_to_client);
    file_process::close(fd_to_client);
}

void list(int fd_to_client, char *buf)
{
    FILE *read_fp{popen("ls", "r")};
    if (read_fp != nullptr)
    {
        if (std::size_t read_num{
                fread(buf, sizeof(char), BUF_SIZE - 1, read_fp)};
            read_num > 0)
        {
            buf[read_num] = 0;

            myftp_head list_reply(MYFTP_HEAD_TYPE::LIST_REPLY, 1,
                                  MYFTP_HEAD_SIZE + read_num + 1);

            list_reply.send(fd_to_client);
            file_process::write(fd_to_client, buf, read_num + 1);
        }
        pclose(read_fp);
    }
}

void sha256(int fd_to_client, char *buf, std::uint32_t file_name_length)
{
    file_process::read(fd_to_client, buf, file_name_length);

    std::string cmd{"sha256sum "};
    cmd += "\"";
    cmd += buf;
    cmd += "\"";

    std::string path{"./"};
    path += buf;

    if (!std::filesystem::exists(path))
        SHA_REPLAY_FAIL.send(fd_to_client);
    else
    {
        FILE *read_fp{popen(cmd.c_str(), "r")};
        if (read_fp != nullptr)
        {
            if (std::size_t read_num{
                    fread(buf, sizeof(char), BUF_SIZE - 1, read_fp)};
                read_num > 0)
            {
                buf[read_num] = 0;

                SHA_REPLAY_SUCCESS.send(fd_to_client);

                myftp_head sha_reply_head(MYFTP_HEAD_TYPE::FILE_DATA, 1,
                                          MYFTP_HEAD_SIZE + read_num + 1);
                sha_reply_head.send(fd_to_client);
                file_process::write(fd_to_client, buf, read_num + 1);
            }
            pclose(read_fp);
        }
    }
}
