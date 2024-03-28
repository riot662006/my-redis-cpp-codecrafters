#include "utils.h"
#include "Data.h"
#include "Conn.h"
#include "Server.h"

#pragma once

using CommandFunctionType = std::function<std::string(Server*, Conn*, const std::queue<Data*>&)>;

class CommandError : public std::exception {
    std::string cmdName, msg;
public:
    CommandError(const std::string& _cmd, const std::string& _msg) : cmdName(_cmd), msg(_msg) {};

    const char* what() const noexcept override;
};

std::string PingCommand(Server* server, Conn* conn, std::queue<Data*> args);
std::string EchoCommand(Server* server, Conn* conn, std::queue<Data*> args);
std::string SetCommand(Server* server, Conn* conn, std::queue<Data*> args);
std::string GetCommand(Server* server, Conn* conn, std::queue<Data*> args);
std::string InfoCommand(Server* server, Conn* conn, std::queue<Data*> args);
std::string ReplconfCommand(Server* server, Conn* conn, std::queue<Data*> args);

class CommandManager {
private:
    Server* server;
    Conn* curConn;
    std::unordered_map<std::string, CommandFunctionType> functionMap;
public:
    CommandManager(Server* _server);

    std::string runCommand(Data* cmd);
    void tryProcess(Conn* conn);
};