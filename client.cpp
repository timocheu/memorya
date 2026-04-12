#include <cstdio>
#include <cstring>
#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>

void die(const char *message);
void msg(const char *message);

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1
    
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("connect");
    }

    int32_t err = query(fd, "Hello1");
    if (err) {
        goto L_DONE;
    }

    err = query(fd, "Hello2");
    if (err) {
        goto L_DONE;
    }
L_DONE:
    close(fd);
    return 0;
}

const size_t k_max_msg = 4096;

static int32_t query(int fd, const char *text) {
    uint32_t len = (uint32_t)strlen(text);
    if (len > k_max_msg) {
        return -1; // Err message too long
    }

    // send request
    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], text, len);
    if (int32_t err = write_all(fd, wbuf, 4 + len)) {
        return err;
    }

    // 4 bytes
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }
    memcpy(&len, rbuf, 4);
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return error;
    }

    printf("server says: %.*s\n", len, &rbuf[4]);
    return 0;
}

void die(const char *message) {
    perror(message);
    exit(1);
}

void msg(const char *message) {
    std::cerr << message << std::endl;
}
