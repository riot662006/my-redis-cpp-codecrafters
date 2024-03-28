#include "utils.h"


char emptyRDB[EMPTY_RDB_SIZE] = {'\x52', '\x45', '\x44', '\x49', '\x53', '\x30', '\x30', '\x31', '\x31', '\xfa', 
'\x09', '\x72', '\x65', '\x64', '\x69', '\x73', '\x2d', '\x76', '\x65', '\x72', 
'\x05', '\x37', '\x2e', '\x32', '\x2e', '\x30', '\xfa', '\x0a', '\x72', '\x65', 
'\x64', '\x69', '\x73', '\x2d', '\x62', '\x69', '\x74', '\x73', '\xc0', '\x40', 
'\xfa', '\x05', '\x63', '\x74', '\x69', '\x6d', '\x65', '\xc2', '\x6d', '\x08', 
'\xbc', '\x65', '\xfa', '\x08', '\x75', '\x73', '\x65', '\x64', '\x2d', '\x6d', 
'\x65', '\x6d', '\xc2', '\xb0', '\xc4', '\x10', '\x00', '\xfa', '\x08', '\x61', 
'\x6f', '\x66', '\x2d', '\x62', '\x61', '\x73', '\x65', '\xc0', '\x00', '\xff', 
'\xf0', '\x6e', '\x3b', '\xfe', '\xc0', '\xff', '\x5a', '\xa2'};

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

size_t Buffer::find(char c) {
    for (size_t ptr = 0; ptr < this->bytes; ++ptr) {
        if (this->buf[ptr] == c) return ptr;
    }
    return -1;
}

Timepoint getNow() {
    return std::chrono::system_clock::now();
}