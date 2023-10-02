#ifndef SOCKET_HXX
#define SOCKET_HXX

#include <netdb.h>

constexpr int optval{1};
int Open_listen_fd(const char *);
int Open_clientfd(const char *hostname, const char *port);
int Accept(int s, sockaddr *addr, socklen_t *addrlen);
#endif
