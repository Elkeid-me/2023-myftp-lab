#include "error_handle.hxx"
#include <cstddef>
#include <unistd.h>

namespace file_process
{
    void close(int fd)
    {
        if (::close(fd) == -1)
            unix_error("Function `close' error");
    }

    std::size_t read(int fd, char *buf, std::size_t size)
    {
        ssize_t ret{::read(fd, buf, size)};
        if (ret < 0)
            unix_error("Function `read' error");
        return ret;
    }

    std::size_t write(int fd, const char *buf, std::size_t size)
    {
        ssize_t ret{::write(fd, buf, size)};
        if (ret < 0)
            unix_error("Function `write' error");
        return ret;
    }
}
