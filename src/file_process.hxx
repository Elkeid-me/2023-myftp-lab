#ifndef FILE_PROCESS_HXX
#define FILE_PROCESS_HXX

#include <cstddef>
#include <sys/types.h>

namespace file_process
{
    void close(int fd);
    [[nodiscard]] std::size_t read(int fd, char *buf, std::size_t size);
    [[nodiscard]] std::size_t write(int fd, const char *buf, std::size_t size);
}

#endif
