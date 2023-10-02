#include "error_handle.hxx"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <netdb.h>

void unix_error(const char *msg)
{
    std::fprintf(stderr, "%s: %s\n", msg, std::strerror(errno));
}

void posix_error(int code, const char *msg)
{
    std::fprintf(stderr, "%s: %s\n", msg, std::strerror(code));
}

void gai_error(int code, const char *msg)
{
    std::fprintf(stderr, "%s: %s\n", msg, gai_strerror(code));
}
