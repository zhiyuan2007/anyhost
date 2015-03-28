#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include "zip_socket.h"
#include "zip_addr.h"
#include "log_utils.h"

void
socket_open(socket_t *s, int family, int type, int protocol)
{
    SOCKET_FD(s) = socket(family, type, protocol);
    ASSERT(SOCKET_FD(s) > 0, "socket %d isn't valid", s->fd);
    SOCKET_TYPE(s) = type;
    SOCKET_FAMILY(s) = family;
}

void
socket_close(socket_t *s)
{
    close(SOCKET_FD(s));
}

int
socket_set_unblock(socket_t *s, bool is_block)
{
    int sock_flags = fcntl(SOCKET_FD(s), F_GETFL, 0);
    ASSERT(sock_flags >=0, "get socket flag failed %d", sock_flags);
    if (is_block)
        return fcntl(SOCKET_FD(s), F_SETFL, sock_flags | O_NONBLOCK);
    else
        return fcntl(SOCKET_FD(s), F_SETFL, sock_flags &(~O_NONBLOCK));
}

int
socket_set_addr_reusable(socket_t *s)
{
    int flag = 1;
    return setsockopt(SOCKET_FD(s), SOL_SOCKET, SO_REUSEADDR,
                                                        &flag, sizeof(int));
}

inline int 
socket_set_cork(socket_t *s, int on_or_off)
{
#ifdef TCP_CORK
    int flag = on_or_off;
    return setsockopt(SOCKET_FD(s), IPPROTO_TCP,TCP_CORK, &flag, sizeof(flag));
#endif
}
int
socket_set_rdtimeout(socket_t *s, int secs)
{
    struct timeval tm;
    tm.tv_sec = secs;
    tm.tv_usec = 0;
    return setsockopt(SOCKET_FD(s), SOL_SOCKET, SO_RCVTIMEO, &tm, sizeof(tm));
}

int
socket_set_notimewait(socket_t *s)
{
    struct linger nowait_linger;
    nowait_linger.l_onoff  = 1;
    nowait_linger.l_linger = 0;
    return setsockopt(SOCKET_FD(s),
                      SOL_SOCKET,
                      SO_LINGER,
                      (const void *)&nowait_linger,
                      sizeof(struct linger));
}

int
socket_connect(socket_t *s, const addr_t *server_addr)
{
    return connect(SOCKET_FD(s),
                   addr_to_sockaddr(server_addr),
                   addr_get_socklen(server_addr));
}


int
socket_connect_with_timeout(socket_t *s,
                            const addr_t *server_addr,
                            uint32_t sec_to_wait)
{
    int ret = socket_connect(s, server_addr);
    if (ret != 0)
    {
        if (EINPROGRESS == errno)
        {
            fd_set set;
            FD_ZERO(&set);
            FD_SET(SOCKET_FD(s), &set);
            struct timeval tv;
            tv.tv_sec = sec_to_wait;
            tv.tv_usec = 0;
            if (select(SOCKET_FD(s) + 1, NULL, &set, NULL, &tv) > 0)
                ret = 0;
            else
                ret = 1;
        }
    }

    return ret;
}

int
socket_bind(socket_t *s, const addr_t *addr)
{
    return bind(SOCKET_FD(s), addr_to_sockaddr(addr), addr_get_socklen(addr));
}

int
socket_listen(socket_t *s, int pending_request)
{
    return listen(SOCKET_FD(s), pending_request);
}

socket_t *
socket_accept(socket_t *s)
{
    const char *default_addr =
                        (SOCKET_FAMILY(s) == AF_INET6) ? "::0" : "0.0.0.0";
    addr_t peer_addr;
    addr_init(&peer_addr, default_addr, 0);
    socklen_t sock_len = addr_get_socklen(&peer_addr);
    int new_fd = accept(SOCKET_FD(s),
                       (struct sockaddr *)addr_to_sockaddr(&peer_addr),
                       &sock_len);

    socket_t *new_socket;
    DIG_MALLOC(new_socket,sizeof(socket_t));
    SOCKET_FD(new_socket) = new_fd;
    SOCKET_TYPE(new_socket) = SOCKET_TYPE(s);

    return new_socket;
}

int
socket_read(socket_t *s, void *buffer, size_t len)
{
    if (SOCKET_TYPE(s) == SOCK_STREAM)
    {
        char *read_pos = buffer;
        size_t left_len = len;
        ssize_t read_len = 0;
        while (left_len > 0)
        {
            if ((read_len = read(SOCKET_FD(s), read_pos, left_len)) < 0)
            {
                if (errno == EINTR)
                    read_len = 0;
                else if (errno == EAGAIN)
                    break;
                else
                    return -1;
            }
            else if (read_len == 0)
                break;

            left_len -= read_len;
            read_pos += read_len;
        }

        return len - left_len;
    }
    else
    {
        return recv(SOCKET_FD(s), buffer, len, 0);
    }

}

int
socket_read_from(socket_t *s, void *buffer, size_t len, addr_t *source_addr)
{
    if (SOCKET_TYPE(s) == SOCK_STREAM)
        ASSERT(0, "read form is used for udp socket\n");

    const char *default_addr =
                        (SOCKET_FAMILY(s) == AF_INET6) ? "::0" : "0.0.0.0";
    addr_init(source_addr, default_addr, 0);
    socklen_t sock_len = addr_get_socklen(source_addr);
    return recvfrom(SOCKET_FD(s),
                    buffer,
                    len,
                    0,
                    addr_to_sockaddr(source_addr),
                    &sock_len);
}

int
socket_write(socket_t *s, void *buffer, size_t len)
{
    if (SOCKET_TYPE(s) == SOCK_STREAM)
    {
        size_t left_len = len;
        ssize_t written_len = 0;
        char *write_pos = buffer;
        while (left_len > 0)
        {
            if ((written_len = write(SOCKET_FD(s), write_pos, left_len)) <= 0)
            {
                if (written_len < 0 && errno == EINTR)
                    written_len = 0;
                else
                    return -1;
            }
            left_len -= written_len;
            write_pos += written_len;
        }

        return len;
    }
    else
    {
        return send(SOCKET_FD(s), buffer, len, 0);
    }
}

int
socket_write_to(socket_t *s, void *buffer, size_t len, const addr_t *dest_addr)
{
    if (SOCKET_TYPE(s) == SOCK_STREAM)
        ASSERT(0, "write to is used for udp socket");

    return sendto(SOCKET_FD(s),
                  buffer,
                  len,
                  0,
                  addr_to_sockaddr(dest_addr),
                  addr_get_socklen(dest_addr));
}

inline int
socket_get_fd(const socket_t *s)
{
    return SOCKET_FD(s);
}

inline int
socket_get_type(const socket_t *s)
{
    return SOCKET_TYPE(s);
}

inline int
socket_get_peer_addr(const socket_t *s, addr_t *peer_addr)
{
    const char *default_addr =
                        (SOCKET_FAMILY(s) == AF_INET6) ? "::0" : "0.0.0.0";
    addr_init(peer_addr, default_addr, 0);
    socklen_t sock_len = addr_get_socklen(peer_addr);
    return getpeername(SOCKET_FD(s),
                       (struct sockaddr *)addr_to_sockaddr(peer_addr),
                       &sock_len);
}
