#include "Conn.h"
#pragma once

class Server {
    int fd;
    struct sockaddr_in addr;

    int state = STATE_READ;
public:
    std::unordered_map<int, Conn*> conns;

    Server();
    ~Server();
    void acceptNewConn();

    int getFd() {return this->fd;}
    bool isRunning() {return this->state != STATE_END;}
    void closeServer() {close(this->fd); this->state = STATE_END;}

    Conn* getConn(int fd) {return conns[fd];}
    void deleteConnAt(int fd) {delete conns[fd]; conns.erase(fd);}
};

void serverConfig(Server* server, int argc, char ** argv);