#ifndef SOCKET_HXX
#define SOCKET_HXX

#include <netdb.h>

namespace socket_process
{
    constexpr int optval{1};
    int open_listen_fd(const char *port);
    int open_client_fd(const char *hostname, const char *port);
    int accept(int s, sockaddr *addr, socklen_t *addrlen);
}

#endif
