#include "utils.h"
#include "Data.h"
#include "Conn.h"
#include "Server.h"

#pragma once

using FunctionType = std::function<std::string(Server*, const std::vector<Data*>&)>;

class CommandError : public std::exception {
    std::string cmdName, msg;
public:
    CommandError(const std::string& _cmd, const std::string& _msg) : cmdName(_cmd), msg(_msg) {};

    const char* what() const noexcept override;
};

std::string PingCommand(Server* server, std::vector<Data*> args);
std::string EchoCommand(Server* server, std::vector<Data*> args);

class CommandManager {
private:
    Server* server;
    std::unordered_map<std::string, FunctionType> functionMap;
public:
    CommandManager(Server* _server);

    std::string runCommand(Data* cmd);
};