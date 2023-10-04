#include "error_handle.hxx"
#include <cerrno>
#include <cstddef>
#include <unistd.h>

namespace file_process
{
    void close(int fd)
    {
        if (::close(fd) == -1)
            error_handle::unix_error("Function `close' error");
    }

    [[nodiscard]] ssize_t read(int fd, char *buf, std::size_t size)
    {
        ssize_t ret{::read(fd, buf, size)};
        if (ret < 0)
            error_handle::unix_error("Function `read' error");
        return ret;
    }

    [[nodiscard]] ssize_t write(int fd, const char *buf, std::size_t size)
    {
        ssize_t ret{::write(fd, buf, size)};
        if (ret < 0)
            error_handle::unix_error("Function `write' error");
        return ret;
    }

    // ssize_t write(int fd, const char *buf, std::size_t n)
    // {
    //     std::size_t n_left{n};
    //     ssize_t n_written;

    //     while (n_left > 0)
    //     {
    //         if ((n_written = ::write(fd, buf, n_left)) <= 0)
    //         {
    //             if (errno == EINTR)
    //                 n_written = 0;
    //             else
    //                 return -1;
    //         }
    //         n_left -= n_written;
    //         buf += n_written;
    //     }
    //     return n;
    // }

    // ssize_t read(int fd, char *buf, std::size_t n)
    // {
    //     std::size_t n_left{n};
    //     ssize_t nread;

    //     while (n_left > 0)
    //     {
    //         if ((nread = ::read(fd, buf, n_left)) < 0)
    //         {
    //             if (errno == EINTR)
    //                 nread = 0;
    //             else
    //                 return -1;
    //         }
    //         else if (nread == 0)
    //             break;
    //         n_left -= nread;
    //         buf += nread;
    //     }
    //     return n - n_left;
    // }
}
