#include "utils.h"

#pragma once

struct Conn {
    int fd;
    int state = STATE_READ;

    Buffer* rbuf;
    std::deque<std::string> writeQueue;

    Conn(int f) : fd(f) {
        rbuf = new Buffer();
    }

    ~Conn() {delete rbuf;}
};

void tryRead(Conn* conn);
void tryWrite(Conn* conn);
