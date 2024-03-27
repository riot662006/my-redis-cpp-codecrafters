#include "utils.h"
#include "Parser.h"

#pragma once

struct Conn {
    int fd;
    int state = STATE_READ;

    Buffer* rbuf; CommandParser* parser;
    Data* processMem = nullptr;
    std::deque<std::string> writeQueue;

    Conn(int f) : fd(f) {
        rbuf = new Buffer();
        parser = new CommandParser(rbuf);
    }

    ~Conn() {delete rbuf; delete parser;}
};

void tryRead(Conn* conn);
void tryWrite(Conn* conn);
void tryAddResponse(Conn* conn, std::string response);
