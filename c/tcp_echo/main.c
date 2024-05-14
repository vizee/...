#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define FATAL() do { printf("%s:%d: %s\n", __FUNCTION__, __LINE__, strerror(errno)); exit(EXIT_FAILURE); } while (0)

#define MAX_EVENTS 64
#define EPOLL_EVENTS_NUM 6

struct io_context {
    int kind;
    int fd;
    void *data;
    int prd;
    int pwr;
};

static int EPOLL_FD = -1;

void del_epoll_ctx(struct io_context *ctx)
{
    struct epoll_event ev;

    if (epoll_ctl(EPOLL_FD, EPOLL_CTL_DEL, ctx->fd, &ev) < 0) {
        FATAL();
    }
    free(ctx);
}

struct io_context *add_epoll_ctx(int kind, int fd, int rw, void *data) {
    struct io_context *ctx = (struct io_context *)malloc(sizeof(struct io_context));
    ctx->kind = kind;
    ctx->fd = fd;
    ctx->data = data;
    ctx->prd = 0;
    ctx->pwr = 0;

    struct epoll_event ee;
    ee.data.ptr = ctx;
    ee.events = EPOLLET;
    if (rw & 1) {
        ee.events |= EPOLLIN | EPOLLRDHUP;
    }
    if (rw & 2) {
        ee.events |= EPOLLOUT;
    }

    if (epoll_ctl(EPOLL_FD, EPOLL_CTL_ADD, fd, &ee) < 0) {
        FATAL();
    }
    return ctx;
}

#define CLIENT_BUF_SIZE 4096

struct client {
    char buf[CLIENT_BUF_SIZE];
    size_t sz;
    size_t wr;
};

void close_client(struct io_context *ctx) {
    free(ctx->data);
    int fd = ctx->fd;
    del_epoll_ctx(ctx);
    close(fd);
}

void handle_client_io(struct io_context *ctx) {
    printf("client io: fd=%d\n", ctx->fd);

    struct client *c = (struct client *)ctx->data;

    if (ctx->prd) {
        while (c->sz < CLIENT_BUF_SIZE) {
            ssize_t n = read(ctx->fd, c->buf + c->sz, CLIENT_BUF_SIZE- c->sz);
            if (n < 0) {
                if (errno == EAGAIN) {
                    ctx->prd = 0;
                    break;
                }
                printf("client error: fd=%d err=%s\n", ctx->fd, strerror(errno));
                close_client(ctx);
                return;
            }
            if (n == 0) {
                printf("client error: fd=%d EOF\n", ctx->fd);
                close_client(ctx);
                return;
            }
            c->sz += (size_t)n;
        }
    }
    if (ctx->pwr) {
        while (c->wr < c->sz) {
            ssize_t n = write(ctx->fd, c->buf + c->wr, c->sz - c->wr);
            if (n < 0) {
                if (errno == EAGAIN) {
                    ctx->pwr = 0;
                    break;
                }
                printf("client %d error: %s\n", ctx->fd, strerror(errno));
                close_client(ctx);
                return;
            }
            c->wr += (size_t)n;
        }
        if (c->wr == c->sz) {
            c->wr = 0;
            c->sz = 0;
        }
    }
}

void print_sockaddr(const char *msg, struct sockaddr *sa) {
    char ipstr[INET6_ADDRSTRLEN];
    void *addr;
    int port;

    if (sa->sa_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)sa;
        addr = &(s->sin_addr);
        port = ntohs(s->sin_port);
    } else if (sa->sa_family == AF_INET6) {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)sa;
        addr = &(s->sin6_addr);
        port = ntohs(s->sin6_port);
    } else {
        return;
    }

    inet_ntop(sa->sa_family, addr, ipstr, sizeof(ipstr));
    printf("%s: %s:%d\n", msg, ipstr, port);
}

void handle_new_client(int cfd, struct sockaddr* addr, socklen_t addr_len) {
    print_sockaddr("accept new client", addr);
    struct client *c = malloc(sizeof(struct client));
    c->sz = 0;
    c->wr = 0;
    add_epoll_ctx(2, cfd, 3, c);
}

void accept_client(struct io_context *ctx) {
    struct sockaddr addr;
    socklen_t addr_len;

    for (;;) {
        addr_len = sizeof(addr);
        int cfd = accept4(ctx->fd, &addr, &addr_len, SOCK_NONBLOCK);
        if (cfd < 0) {
            if (errno == EAGAIN) {
                ctx->prd = 0;
                return;
            }
            FATAL();
        }

        int on = 1;
        if (setsockopt(cfd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(int)) < 0) {
            FATAL();
        }
        int secs = 15;
        if (setsockopt(cfd, IPPROTO_TCP, TCP_KEEPINTVL, &secs, sizeof(int)) < 0) {
            FATAL();
        }
        if (setsockopt(cfd, IPPROTO_TCP, TCP_KEEPIDLE, &secs, sizeof(int)) < 0) {
            FATAL();
        }

        handle_new_client(cfd, &addr, addr_len);
    }
}

void process_io(struct io_context *ctx) {
    printf("process_io: kind=%d fd=%d prd=%d pwr=%d\n", ctx->kind, ctx->fd, ctx->prd, ctx->pwr);

    switch (ctx->kind) {
    case 1:
        accept_client(ctx);
        break;
    case 2:
        handle_client_io(ctx);
        break;
    }
}

int listen_port(int port) {
    int fd = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0);
    if (fd < 0) {
        FATAL();
    }

    struct sockaddr_in sin;
    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) < 0) {
        FATAL();
    }
    int on = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        FATAL();
    }

    if (listen(fd, SOMAXCONN) < 0) {
        FATAL();
    }

    return fd;
}

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "tcp_echo port\n");
        return 1;
    }
    int port = atoi(argv[1]);

    static uint32_t EPOLL_EVENTS[EPOLL_EVENTS_NUM] = {EPOLLIN, EPOLLOUT, EPOLLRDHUP, EPOLLPRI, EPOLLERR, EPOLLHUP};
    static const char *EPOLL_EVENT_NAMES[EPOLL_EVENTS_NUM] = {"EPOLLIN", "EPOLLOUT", "EPOLLRDHUP", "EPOLLPRI", "EPOLLERR", "EPOLLHUP"};

    int epfd = epoll_create1(0);
    if (epfd == -1) {
        FATAL();
    }
    EPOLL_FD = epfd;

    int lfd = listen_port(port);
    printf("listen: %d\n", port);
    add_epoll_ctx(1, lfd, 1, NULL);

    struct epoll_event events[MAX_EVENTS];
    for (;;) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            FATAL();
        }
        for (int i = 0; i < n; i++) {
            struct io_context *ctx = (struct io_context *)events[i].data.ptr;

            uint32_t ev = events[i].events;

            printf("epoll: event ctx %p\n", ctx);
            for (int j = 0; j < EPOLL_EVENTS_NUM; j++) {
                if (ev & EPOLL_EVENTS[j]) {
                    printf("    - %s\n", EPOLL_EVENT_NAMES[j]);
                }
            }

            const uint32_t READ_EVENTS = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
            if (ev & READ_EVENTS) {
                ctx->prd = 1;
            }
            const uint32_t WRITE_EVENTS = EPOLLOUT | EPOLLHUP | EPOLLERR;
            if (ev & WRITE_EVENTS) {
                ctx->pwr = 1;
            }

            process_io(ctx);
        }
    }

    return 0;
}
