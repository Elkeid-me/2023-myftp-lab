#ifndef TOOLS_HXX
#define TOOLS_HXX

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <regex>
#include <string_view>

constexpr std::size_t MAGIC_NUMBER_LENGTH{6};

constexpr std::string_view MYFTP_PROTOCOL{"\xc1\xa1\x10"
                                          "ftp"};
constexpr std::size_t BUF_SIZE{16384};

constexpr auto REGEX_FLAG{std::regex_constants::icase |
                          std::regex_constants::ECMAScript |
                          std::regex_constants::optimize};
const std::regex PORT_PATTERN{"[0-9]+", REGEX_FLAG};
const std::regex IPv4_PATTERN{R"([0-9]{1,3}(\.[0-9]{1,3}){3})", REGEX_FLAG};
const std::regex IPv6_PATTERN{R"(([0-9]|[a-f])(:([0-9]|[a-f])){7})",
                              REGEX_FLAG};

enum class MYFTP_HEAD_TYPE : std::uint8_t
{
    INVALID = 0x00,

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
    std::uint8_t m_status;
    std::uint32_t m_length;
    bool is_valid() const;

public:
    myftp_head(MYFTP_HEAD_TYPE type, std::uint8_t status,
               std::uint32_t length_host_endian);
    myftp_head() = default;

    void pack(MYFTP_HEAD_TYPE type, std::uint8_t status,
              std::uint32_t length_host_endian);

    MYFTP_HEAD_TYPE get_type() const;
    std::uint8_t get_status() const;
    std::uint32_t get_length() const;
    std::uint32_t get_payload_length() const;

    [[nodiscard]] bool get(int fd_to_host);
    [[nodiscard]] bool send(int fd_to_host) const;
};

class FILE_ptr
{
private:
    std::FILE *m_ptr = nullptr;

public:
    FILE_ptr(std::FILE *ptr);
    bool is_valid() const;
    std::FILE *get_ptr();
    ~FILE_ptr();
};

constexpr std::size_t MYFTP_HEAD_SIZE{sizeof(myftp_head)};
static_assert(MYFTP_HEAD_SIZE == 12);

[[nodiscard]] bool send_file(int fd_to_host, const char *path, char *buf,
                             std::size_t size);
[[nodiscard]] bool receive_file(int fd_to_host, const char *path, char *buf,
                                std::size_t size);

const myftp_head
    OPEN_CONNECTION_REQUEST(MYFTP_HEAD_TYPE::OPEN_CONNECTION_REQUEST, 1,
                            MYFTP_HEAD_SIZE);
const myftp_head OPEN_CONNECTION_REPLY(MYFTP_HEAD_TYPE::OPEN_CONNECTION_REPLY,
                                       1, MYFTP_HEAD_SIZE);

const myftp_head LIST_REQUEST(MYFTP_HEAD_TYPE::LIST_REQUEST, 1,
                              MYFTP_HEAD_SIZE);

const myftp_head GET_REPLY_SUCCESS(MYFTP_HEAD_TYPE::GET_REPLY, 1,
                                   MYFTP_HEAD_SIZE);
const myftp_head GET_REPLY_FAIL(MYFTP_HEAD_TYPE::GET_REPLY, 0, MYFTP_HEAD_SIZE);

const myftp_head PUT_REPLY(MYFTP_HEAD_TYPE::PUT_REPLY, 1, MYFTP_HEAD_SIZE);

const myftp_head SHA_REPLAY_SUCCESS(MYFTP_HEAD_TYPE::SHA_REPLY, 1,
                                    MYFTP_HEAD_SIZE);
const myftp_head SHA_REPLAY_FAIL(MYFTP_HEAD_TYPE::SHA_REPLY, 0,
                                 MYFTP_HEAD_SIZE);

const myftp_head QUIT_REQUEST(MYFTP_HEAD_TYPE::QUIT_REQUEST, 1,
                              MYFTP_HEAD_SIZE);
const myftp_head QUIT_REPLY(MYFTP_HEAD_TYPE::QUIT_REPLY, 1, MYFTP_HEAD_SIZE);

#endif
