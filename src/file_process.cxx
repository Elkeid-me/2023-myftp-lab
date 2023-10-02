#include "error_handle.hxx"
#include <cstddef>
#include <unistd.h>

void Close(int fd)
{
    if (close(fd) == -1)
        unix_error("Function `close' error");
}

void Write(int fd, const char *buf, std::size_t size)
{
    if (write(fd, buf, size) < 0)
        unix_error("Function `write' error");
}
