#ifndef TOOLS_HXX
#define TOOLS_HXX

#include <cstddef>
#include <cstdint>
#include <string_view>

constexpr uint64_t MAGIC_NUMBER_LENGTH{6};

constexpr std::string_view MYFTP_PROTOCOL{"\xc1\xa1\x10"
                                        "ftp"};

enum class MYFTP_HEAD_TYPE : unsigned char
{
    OPEN_CONNECTION_REQUEST = 0xa1,
    OPEN_CONNECTION_REPLY = 0xa2,

    LIST_REQUEST = 0xa3,
    LIST_REPLY = 0xa4,

    GET_REQUEST = 0xa5,
    GET_REPLY = 0xa6,

    PUT_REQUEST = 0xa7,
    PUT_REPLY = 0xa8,

    SHA_REQUEST = 0xa9,
    SHA_REPLY = 0xaa,

    QUIT_REQUEST = 0xab,
    QUIT_REPLY = 0xac,

    FILE_DATA = 0xFF,
};

class [[gnu::packed]] myftp_head
{
private:
    char m_protocol[MAGIC_NUMBER_LENGTH];
    MYFTP_HEAD_TYPE m_type;
    unsigned char m_status;
    std::uint32_t m_length;

public:
    myftp_head(MYFTP_HEAD_TYPE type, unsigned char status,
               std::uint32_t length_host_endian);

    bool is_valid() const;

    MYFTP_HEAD_TYPE get_type() const;
    unsigned char get_status() const;
    std::uint32_t get_length() const;
    std::uint32_t get_file_length() const;
    std::uint32_t get_file_name_length() const;

    void get(int fd_to_host);
    void send(int fd_to_host) const;
};

const myftp_head
    OPEN_CONNECTION_REPLY_HEAD(MYFTP_HEAD_TYPE::OPEN_CONNECTION_REPLY, 1, 12);

#endif
