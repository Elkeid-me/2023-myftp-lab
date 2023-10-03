#ifndef FILE_PROCESS_HXX
#define FILE_PROCESS_HXX

#include <cstddef>

namespace file_process
{
    void close(int fd);
    std::size_t read(int fd, char *buf, std::size_t size);
    std::size_t write(int fd, const char *buf, std::size_t size);
}

#endif
