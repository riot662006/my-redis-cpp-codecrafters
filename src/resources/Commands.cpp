#include "Commands.h"

const char* CommandError::what() const noexcept {
    static std::string fullMsg;
    fullMsg = "Error: command error [ " + this->cmdName + " ] => " + this->msg;

    return fullMsg.c_str();
}

CommandManager::CommandManager(Server* _server) {
    this->server = _server;

    this->functionMap.insert({"ping", PingCommand});
    this->functionMap.insert({"echo", EchoCommand});
    this->functionMap.insert({"set", SetCommand});
    this->functionMap.insert({"get", GetCommand});
}

std::string PingCommand(Server* server, std::vector<Data*> args) {
    if (args.size() != 0) throw CommandError("ping", "expected 0 arguments, got " + std::to_string(args.size()));
    return "+PONG\r\n";
}

std::string EchoCommand(Server* server, std::vector<Data*> args) {
    if (args.size() != 1) throw CommandError("ping", "expected 1 arguments, got " + std::to_string(args.size()));
    
    return args[0]->toRespString();
}

std::string SetCommand(Server* server, std::vector<Data*> args) {
    if (args.size() != 2) throw CommandError("ping", "expected 2 arguments, got " + std::to_string(args.size()));

    server->setData(args[0]->getStringData(), args[1]);
    return "+OK\r\n";
}

std::string GetCommand(Server* server, std::vector<Data*> args) {
    if (args.size() != 1) throw CommandError("ping", "expected 1 arguments, got " + std::to_string(args.size()));

    return server->getData(args[0]->getStringData())->toRespString();
}

std::string CommandManager::runCommand(Data* cmd) {
    std::string cmdName = cmd->getArrayData()[0]->getStringData();
    std::vector<Data*> args;
    for (int i = 1; i < cmd->getArrayData().size(); ++i) {
        args.push_back(cmd->getArrayData()[i]);
    }
    
    if (this->functionMap.find(cmdName) == this->functionMap.end()) throw CommandError("_", "command Name '" + cmdName + "' does not exist.");
    std::string response = this->functionMap[cmdName](this->server, args);

    return response;
}