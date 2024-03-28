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
    this->functionMap.insert({"info", InfoCommand});
}

std::string PingCommand(Server* server, std::queue<Data*> args) {
    if (args.size() != 0) throw CommandError("ping", "expected 0 arguments, got " + std::to_string(args.size()));
    return "+PONG\r\n";
}

std::string EchoCommand(Server* server, std::queue<Data*> args) {
    if (args.size() != 1) throw CommandError("echo", "expected 1 arguments, got " + std::to_string(args.size()));
    
    return args.front()->toRespString();
}

std::string SetCommand(Server* server, std::queue<Data*> args) {
    if (args.size() < 2) throw CommandError("set", "expected 2 or more arguments, got " + std::to_string(args.size()));

    std::string key = args.front()->getStringData(); args.pop();
    Data* value = args.front(); args.pop();

    std::string opt;
    try {
        while (!args.empty()) {
            opt = args.front()->getStringData(); args.pop();
            if (opt == "px") {
                size_t milli = std::stoi(args.front()->getStringData());
                value->addExpiry(server->getLastPollTime(), milli);
                args.pop();
            }
        }
    } catch (CommandError& e) {
        throw e;
    } catch (std::exception& e) {
        std::string errorMsg = "error occured while parsing options";

        if (opt.length() > 0) errorMsg += "'" + opt + "'";
        errorMsg += "\n" + std::string(e.what());

        throw CommandError("set", errorMsg);
    }

    server->setData(key, value);
    return "+OK\r\n";
}

std::string GetCommand(Server* server, std::queue<Data*> args) {
    if (args.size() != 1) throw CommandError("get", "expected 1 arguments, got " + std::to_string(args.size()));
    Data* data = server->getData(args.front()->getStringData());

    if (data->hasExpired()) {
        server->delData(args.front()->getStringData());
        return "$-1\r\n"; // null bulk string
    }

    return data->toRespString();
}

std::string InfoCommand(Server* server, std::queue<Data*> args) {
    if (args.size() != 1) throw CommandError("info", "expected 1 arguments, got " + std::to_string(args.size()));
    std::string infoSection = args.front()->getStringData();

    if (infoSection == "replication") {
        return Data(server->getReplInfo()).toRespString();
    } else throw CommandError("info", "invalid info section '" + infoSection + "'");
}

std::string CommandManager::runCommand(Data* cmd) {
    std::string cmdName = to_lowercase(cmd->getArrayData()[0]->getStringData());
    std::queue<Data*> args;
    for (int i = 1; i < cmd->getArrayData().size(); ++i) {
        args.push(cmd->getArrayData()[i]);
    }
    
    if (this->functionMap.find(cmdName) == this->functionMap.end()) throw CommandError("_", "command Name '" + cmdName + "' does not exist.");
    std::string response = this->functionMap[cmdName](this->server, args);

    return response;
}