#include "file_process.hxx"
#include "error_handle.hxx"
#include <cerrno>
#include <cstddef>
#include <iostream>
#include <unistd.h>

namespace file_process
{
    [[nodiscard]] static ssize_t robust_write(int fd, const char *buf,
                                              std::size_t size)
    {
        std::size_t n_written_bytes{0};
        while (n_written_bytes != size)
        {
            ssize_t n_written{
                ::write(fd, buf + n_written_bytes, size - n_written_bytes)};
            if (n_written < 0)
            {
                if (errno == EINTR)
                    n_written = 0;
                else
                    return n_written;
            }

            n_written_bytes += n_written;
        }
        return n_written_bytes;
    }

    [[nodiscard]] static ssize_t robust_read(int fd, char *buf,
                                             const std::size_t size)
    {
        std::size_t n_read_bytes{0};
        while (n_read_bytes != size)
        {
            ssize_t n_read{::read(fd, buf + n_read_bytes, size - n_read_bytes)};
            if (n_read < 0)
            {
                if (errno == EINTR)
                    n_read = 0;
                else
                    return n_read;
            }

            n_read_bytes += n_read;
        }
        return n_read_bytes;
    }

    void close(int fd)
    {
        if (::close(fd) == -1)
            error_handle::unix_error("Function `close' error");
    }

    [[nodiscard]] std::size_t read(int fd, char *buf, std::size_t size)
    {
        ssize_t ret{robust_read(fd, buf, size)};
        if (ret < 0)
            error_handle::unix_error("Function `read' error");
        return ret;
    }

    [[nodiscard]] std::size_t write(int fd, const char *buf, std::size_t size)
    {
        ssize_t ret{robust_write(fd, buf, size)};
        if (ret < 0)
            error_handle::unix_error("Function `write' error");
        return ret;
    }
}
