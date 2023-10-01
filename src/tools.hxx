#ifndef TOOLS_HXX
#define TOOLS_HXX
#include <cstdint>

constexpr uint64_t MAGIC_NUMBER_LENGTH{6};

struct [[gnu::packed]] myftp_head
{
    std::uint8_t m_protocol[MAGIC_NUMBER_LENGTH]; /* protocol magic number (6 bytes) */
    std::uint8_t m_type;                          /* type (1 byte) */
    std::uint8_t m_status;                        /* status (1 byte) */
    std::uint32_t m_length;                       /* length (4 bytes) in Big endian*/
};
#endif // TOOLS_HXX
