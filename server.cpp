#include <cstring>
#include <cstdio>
#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>

static void do_something(int connfd);
void die(const char *message);
void msg(const char *message);

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(0);
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));

    rv = listen(fd, SOMAXCONN);
    if (rv) { die("bind()"); }


    while (true) {
        // accept
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
        if (connfd < 0) {
            continue;
        }

        while (true) {
            int32_t err = one_request(connfd);
            if (err) {
                break;
            }
        }

        close(connfd);
    }

    return 0;
}


static void do_something(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);

    if (n < 0) {
        msg("read() err");
        return;
    }
    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
}

const size_t k_max_msg = 4096;

void one_request(int connfd) {
    char rbuf[4 + k_max_msg];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    uint_t len = 0;
    memcpy(&len, rbuf, 4);

    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    printf("client says: %.*\n", len, &rbuf[4]);
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);

    // write the length in the first 4bytes;
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], &reply, len);

    return write_all(connfd, wbuf, 4 + len);
}

void int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;
        }

        assert((size_t)rv <= n);

        n -= (size_t)rv;

        buf += rv;
    }
    return 0;
}

void int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

void die(const char *message) {
    perror(message);
    exit(1);
}

void msg(const char *message) {
    std::cerr << message << std::endl;
}
