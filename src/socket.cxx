#include "socket.hxx"
#include "error_handle.hxx"
#include "file_process.hxx"
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static constexpr int LISTEN_BACKLOG{1024};

static void set_socket_option(int s, int level, int optname, const void *optval,
                              int optlen)
{
    int rc;

    if ((rc = setsockopt(s, level, optname, optval, optlen)) < 0)
        error_handle::unix_error("Setsockopt error");
}

static addrinfo *get_addr_info(const char *host, const char *service,
                               const addrinfo *hints)
{
    addrinfo *result;

    int ecode{getaddrinfo(host, service, hints, &result)};
    if (ecode == 0)
        return result;

    error_handle::gai_error(ecode, "Function `getaddrinfo' error");
    return nullptr;
}

static int open_listen_fd(const char *port)
{
    addrinfo hints;
    std::memset(&hints, 0, sizeof(addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;

    addrinfo *list_head{get_addr_info(nullptr, port, &hints)};

    int listen_fd;
    addrinfo *ptr;

    for (ptr = list_head; ptr; ptr = ptr->ai_next)
    {
        if ((listen_fd = socket(ptr->ai_family, ptr->ai_socktype,
                                ptr->ai_protocol)) >= 0)
        {
            set_socket_option(listen_fd, SOL_SOCKET, SO_REUSEADDR,
                              &socket_process::optval, sizeof(int));

            if (bind(listen_fd, ptr->ai_addr, ptr->ai_addrlen) == 0)
                break;

            file_process::close(listen_fd);
        }
    }

    freeaddrinfo(list_head);
    if (!ptr)
        return -1;

    if (listen(listen_fd, LISTEN_BACKLOG) < 0)
    {
        file_process::close(listen_fd);
        return -1;
    }

    return listen_fd;
}

static int open_client_fd(const char *hostname, const char *port)
{
    int clientfd;
    addrinfo hints, *listp, *p;

    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;
    if (getaddrinfo(hostname, port, &hints, &listp) < 0)
        return -1;

    for (p = listp; p; p = p->ai_next)
    {
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) <
            0)
            continue;

        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
            break;
        file_process::close(clientfd);
    }

    freeaddrinfo(listp);
    if (!p)
        return -1;

    return clientfd;
}

namespace socket_process
{
    int open_listen_fd(const char *port)
    {
        int rc;

        if ((rc = ::open_listen_fd(port)) < 0)
            error_handle::unix_error("Open_listenfd error");
        return rc;
    }

    int open_client_fd(const char *hostname, const char *port)
    {
        int rc;

        if ((rc = ::open_client_fd(hostname, port)) < 0)
            error_handle::unix_error("Open_clientfd error");
        return rc;
    }

    int accept(int s, sockaddr *addr, socklen_t *addrlen)
    {
        int rc;

        if ((rc = ::accept(s, addr, addrlen)) < 0)
            error_handle::unix_error("Accept error");
        return rc;
    }
}
