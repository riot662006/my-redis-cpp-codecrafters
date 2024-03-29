#pragma once

#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <errno.h>
#include <exception>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <random>
#include <netdb.h>
#include <poll.h>
#include <sstream>
#include <stack>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <queue>
#include <vector>

const size_t EMPTY_RDB_SIZE = 88;

using Timepoint = std::chrono::system_clock::time_point;
using Milliseconds = std::chrono::milliseconds;

const int MAX_BUF_SIZE = 1024;
const std::unordered_map<char, std::string> specialCharacters = {
    {'\0', "\\0"},
    {'\r', "\\r"},
    {'\n', "\\n"}
};

enum {
    STATE_END = 0,
    STATE_READ = 1,
    STATE_WRITE = 2
};

namespace DataType {
    enum {
        NONE = 0,
        BULK_STR = 1,
        ARRAY = 2,
    };

    const static std::unordered_map<char, int> identifiers {
        {'*', ARRAY},
        {'$', BULK_STR}
    };
};

class Buffer{
    char* buf;
    size_t bytes = 0;

public:
    Buffer(){
        this->buf = new char[MAX_BUF_SIZE];
    };

    ~Buffer(){
        delete this->buf;
        this->buf = nullptr;
    };

    void clearData(size_t n);
    int addToBuffer(char* data, size_t n);
    
    char* getBuffer() {return this->buf;}
    std::string getString(size_t n, bool readable = false);
    size_t find(char c);
    bool isEmpty() {return this->bytes == 0;}
    char at(size_t n) {return (n < bytes) ? (*(this->buf + n)) : 0;}
    void pop() {this->clearData(1);}
    size_t size() {return bytes;}
};

int fd_set_nb(int fd);
std::string to_readable(std::string str);
std::string to_lowercase(std::string str);
Timepoint getNow();

extern char emptyRDB[EMPTY_RDB_SIZE];
extern Buffer emptyRDBBf;