#include "utils.h"

void Buffer::clearData(size_t n) {
    if (this->bytes < n) {
        memset(this->buf, 0, this->bytes);
        this->bytes = 0;
    } else {
        this->bytes -= n;

        memmove(this->buf, this->buf + n, this->bytes);
        memset(this->buf + this->bytes, 0, n);
    }
}

std::string to_readable(std::string str) {
    std::string res = "";

    for (auto c : str) {
        if (specialCharacters.find(c) != specialCharacters.end()) {
            res.append(specialCharacters.at(c));
        } else {
            res.push_back(c);
        }
    }

    return res;
}

std::string to_lowercase(std::string str) {
    std::string res = "";

    for (auto c : str) {
        res += std::tolower(c);
    }

    return res;
}

int Buffer::addToBuffer(char* data, size_t n) {
    if (MAX_BUF_SIZE - this->bytes < n) return -1;
    memcpy(this->buf + this->bytes, data, n);
    this->bytes += n;
    return 0;
}

std::string Buffer::getString(size_t n, bool readable){
    std::string res(this->buf, this->buf + std::min(n, this->bytes));

    if (readable) return to_readable(res);
    return res;
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

Timepoint getNow() {
    return std::chrono::system_clock::now();
}