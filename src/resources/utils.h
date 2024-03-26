#pragma once

#include <arpa/inet.h>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <poll.h>
#include <stack>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#include <deque>

const int MAX_BUF_SIZE = 1024;

enum {
    STATE_END = 0,
    STATE_READ = 1,
    STATE_WRITE = 2
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
    bool isEmpty() {return this->bytes == 0;}
    size_t size() {return bytes;}
};

int fd_set_nb(int fd);