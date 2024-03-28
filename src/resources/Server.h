#include "Conn.h"
#pragma once

class Server {
    int fd;
    struct sockaddr_in addr;

    int state = STATE_READ;
    std::unordered_map<std::string, Data*> database;
    Timepoint last_poll_time;
public:
    std::unordered_map<int, Conn*> conns;
    short last_poll;

    Server();
    ~Server();
    void acceptNewConn();

    int getFd() {return this->fd;}
    Timepoint getLastPollTime() {return this->last_poll_time;}
    bool isRunning() {return this->state != STATE_END;}
    void updateStatus();

    void closeServer() {close(this->fd); this->state = STATE_END;}

    Conn* getConn(int fd) {return conns[fd];}
    void deleteConnAt(int fd) {close(fd); delete conns[fd]; conns.erase(fd);}

    void setData(std::string key, Data* val);
    Data* getData(std::string key);
    int delData(std::string key);
};

void serverConfig(Server* server, int argc, char ** argv);