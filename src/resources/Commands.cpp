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
}

std::string PingCommand(Server* server, std::vector<Data*> args) {
    if (args.size() != 0) throw CommandError("ping", "expected 0 arguments, got " + std::to_string(args.size()));
    return "+PONG\r\n";
}

std::string EchoCommand(Server* server, std::vector<Data*> args) {
    if (args.size() != 1) throw CommandError("ping", "expected 1 arguments, got " + std::to_string(args.size()));
    
    return args[0]->toRespString();
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