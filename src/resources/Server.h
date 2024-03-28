#include "Conn.h"
#pragma once

class Server {
    int fd;
    int port = 6379;


    struct sockaddr_in addr;

    int state = STATE_READ;
    std::unordered_map<std::string, Data*> database;
    Timepoint last_poll_time;
    
    // replication data
    std::string role = "master";
    int connected_slaves = 0;
    std::string master_replid = "8371b4fb1155b71f4a04d3e1bc3e18c4a990aeeb";
    int master_repl_offset = 0;
    int second_repl_offset = -1;
    int repl_backlog_active = 0;
    int repl_backlog_size = 1048576;
    int repl_backlog_first_byte_offset = 0;
    int repl_backlog_histlen = 0;
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

    void config(int argc, char ** argv);
    void closeServer() {close(this->fd); this->state = STATE_END;}

    Conn* getConn(int fd) {return conns[fd];}
    void deleteConnAt(int fd) {close(fd); delete conns[fd]; conns.erase(fd);}

    void setData(std::string key, Data* val);
    Data* getData(std::string key);
    int delData(std::string key);

    std::string getReplInfo();
};