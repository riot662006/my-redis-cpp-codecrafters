#include "utils.h"

void Buffer::clearData(size_t n) {
    if (this->bytes > n) {
        memset(this->buf, 0, this->bytes);
    } else {
        this->bytes -= n;

        memmove(this->buf, this->buf + n, this->bytes);
        memset(this->buf + this->bytes, 0, n);
    }
}

int Buffer::addToBuffer(char* data, size_t n) {
    if (MAX_BUF_SIZE - this->bytes < n) return -1;
    memcpy(this->buf + this->bytes, data, n);
    this->bytes += n;
    return 0;
}

int fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) {
        return -1;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno) {
        return -1;
    }

    return 0;
}