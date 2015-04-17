#ifndef DIG_SOCKET_H
#define DIG_SOCKET_H

#include "zip_addr.h"

typedef struct msocket {
    int fd;
    int type;
    int family;
}socket_t;

#define SOCKET_FD(s) ((s)->fd)
#define SOCKET_TYPE(s) ((s)->type)
#define SOCKET_FAMILY(s) ((s)->family)
#define SOCKET_PEER_ADDR(s) &((s)->peer_addr)

void
socket_open(socket_t *socket, int socket_family, int socket_type, int protocol);

void
socket_close(socket_t *socket);

int
socket_set_unblock(socket_t *socket, bool is_block);

int
socket_set_rdtimeout(socket_t *socket, int secs);

int
socket_set_addr_reusable(socket_t *socket);


int
socket_connect(socket_t *socket, const addr_t *server);

int
socket_connect_with_timeout(socket_t *s,
                            const addr_t *server_addr,
                            uint32_t sec_to_wait);

int
socket_bind(socket_t *socket, const addr_t *addr);

int
socket_listen(socket_t *socket, int pending_request);

socket_t *
socket_accept(socket_t *socket);

int
socket_read(socket_t *socket, void *buffer, size_t len);

int
socket_read_from(socket_t *socket,
                 void *buffer,
                 size_t len,
                 addr_t *source_addr);

int
socket_write(socket_t *socket, void *buffer, size_t len);

int
socket_write_to(socket_t *s, void *buffer, size_t len, const addr_t *dest_addr);

inline int
socket_get_fd(const socket_t *socket);

inline int
socket_get_type(const socket_t *socket);

inline int
socket_get_peer_addr(const socket_t *socket, addr_t *peer_addr);

#endif
