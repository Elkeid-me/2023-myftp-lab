#include "tools.hxx"
#include "file_process.hxx"
#include <arpa/inet.h>
#include <string_view>

myftp_head::myftp_head(MYFTP_HEAD_TYPE type, unsigned char status,
                       std::uint32_t length_host_endian)
    : m_protocol{'\xc1', '\xa1', '\x10', 'f', 't', 'p'}, m_type{type},
      m_status{status}, m_length{htonl(length_host_endian)}
{
}

bool myftp_head::is_valid() const
{
    std::string_view protocol{m_protocol, 6};
    if (protocol != MYFTP_PROTOCOL)
        return false;

    switch (m_type)
    {
    case MYFTP_HEAD_TYPE::OPEN_CONNECTION_REQUEST:
    case MYFTP_HEAD_TYPE::LIST_REQUEST:
    case MYFTP_HEAD_TYPE::PUT_REPLY:
    case MYFTP_HEAD_TYPE::QUIT_REQUEST:
    case MYFTP_HEAD_TYPE::QUIT_REPLY:
        if (get_length() != 12)
            return false;
        break;

    case MYFTP_HEAD_TYPE::OPEN_CONNECTION_REPLY:
        if (get_status() != 1 || get_length() != 12)
            return false;
        break;

    case MYFTP_HEAD_TYPE::LIST_REPLY:
    case MYFTP_HEAD_TYPE::GET_REQUEST:
    case MYFTP_HEAD_TYPE::PUT_REQUEST:
    case MYFTP_HEAD_TYPE::SHA_REQUEST:
        if (get_length() <= 13)
            return false;
        break;

    case MYFTP_HEAD_TYPE::GET_REPLY:
    case MYFTP_HEAD_TYPE::SHA_REPLY:
        if (get_status() != 0 && get_status() != 1 || get_length() != 12)
            return false;

    case MYFTP_HEAD_TYPE::FILE_DATA:
        if (get_length() < 12)
            return false;
        break;

    default:
        return false;
    }

    return true;
}

MYFTP_HEAD_TYPE myftp_head::get_type() const { return m_type; }
unsigned char myftp_head::get_status() const { return m_status; }
std::uint32_t myftp_head::get_length() const { return ntohl(m_length); }
std::uint32_t myftp_head::get_file_length() const { return get_length() - 12; }
std::uint32_t myftp_head::get_file_name_length() const
{
    return get_length() - 13;
}

void myftp_head::get(int fd_to_host)
{
    Read(fd_to_host, reinterpret_cast<char *>(this), 12);
}

void myftp_head::send(int fd_to_host) const
{
    Write(fd_to_host, reinterpret_cast<const char *>(this), 12);
}
