#include "Commands.h"

const char* CommandError::what() const noexcept {
    static std::string fullMsg;
    fullMsg = "Error: command error [ " + this->cmdName + " ] => " + this->msg;

    return fullMsg.c_str();
}

CommandManager::CommandManager(Server* _server) {
    this->server = _server;

    this->writeCommands.push_back("set");
    this->writeCommands.push_back("del");

    this->functionMap.insert({"ping", PingCommand});
    this->functionMap.insert({"echo", EchoCommand});
    this->functionMap.insert({"set", SetCommand});
    this->functionMap.insert({"get", GetCommand});
    this->functionMap.insert({"info", InfoCommand});

    this->functionMap.insert({"replconf", ReplconfCommand});
    this->functionMap.insert({"psync", PsyncCommand});
}

std::string PingCommand(Server* server, Conn* conn, std::queue<Data*> args) {
    if (args.size() != 0) throw CommandError("ping", "expected 0 arguments, got " + std::to_string(args.size()));
    return "+PONG\r\n";
}

std::string EchoCommand(Server* server, Conn* conn, std::queue<Data*> args) {
    if (args.size() != 1) throw CommandError("echo", "expected 1 arguments, got " + std::to_string(args.size()));
    
    return args.front()->toRespString();
}

std::string SetCommand(Server* server, Conn* conn, std::queue<Data*> args) {
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

std::string GetCommand(Server* server, Conn* conn, std::queue<Data*> args) {
    if (args.size() != 1) throw CommandError("get", "expected 1 arguments, got " + std::to_string(args.size()));
    std::string key = args.front()->getStringData(); args.pop();
    Data* data = server->getData(key);

    if (data->hasExpired()) {
        server->delData(key);
        return "$-1\r\n"; // null bulk string
    }

    return data->toRespString();
}

std::string InfoCommand(Server* server, Conn* conn, std::queue<Data*> args) {
    if (args.size() != 1) throw CommandError("info", "expected 1 arguments, got " + std::to_string(args.size()));
    std::string infoSection = args.front()->getStringData(); args.pop();

    if (infoSection == "replication") {
        return Data(server->getReplInfo()).toRespString();
    } else throw CommandError("info", "invalid info section '" + infoSection + "'");
}

std::string ReplconfCommand(Server* server, Conn* conn, std::queue<Data*> args) {
    if (args.size() != 2) throw CommandError("replconf", "expected 1 arguments, got " + std::to_string(args.size()));
    std::string opt, arg;
    
    while (!args.empty()) {
        opt = args.front()->getStringData(); args.pop();
        arg = args.front()->getStringData(); args.pop();

        if (opt == "listening-port") {
            conn->repl_port = std::stoi(arg);
        } else if (opt == "capa") {
            if (arg == "psync2") {
                conn->isSlave = true;
            } else if (arg == "eof") {
                // inform the server that the client is capable of processing the EOF (end of file) marker during replication.
                // Don't know what to do with this right now
            } else throw CommandError("replconf", "Unknown input. capa can not be '" + arg + "'");
        } else throw CommandError("replconf", "Unknown option '" + opt + "'");
    }

    return "+OK\r\n";
}

std::string PsyncCommand(Server* server, Conn* conn, std::queue<Data*> args) {
    if (args.size() != 2) throw CommandError("psync", "expected 1 arguments, got " + std::to_string(args.size()));
    if (!conn->isSlave) throw CommandError("psync", "Unauthorized command. client is not a replica of server");

    std::string replId = args.front()->getStringData(); args.pop();
    int replOffset = std::stoi(args.front()->getStringData()); args.pop();

    if (replId == "?" && replOffset == -1) {
        conn->writeQueue.push_back("+FULLRESYNC " + server->getReplId() + " " + std::to_string(server->getReplOffest()) + "\r\n");
        return "$" + std::to_string(EMPTY_RDB_SIZE) + "\r\n" + std::string(emptyRDB, emptyRDB + EMPTY_RDB_SIZE);
    } else throw CommandError("psync", "Invalid psync arguments. replId = '" + replId + "', replOffset = " + std::to_string(replOffset));
}

std::string CommandManager::runCommand(Data* cmd) {
    std::string cmdName = to_lowercase(cmd->getArrayData()[0]->getStringData());

    std::queue<Data*> args;
    for (int i = 1; i < cmd->getArrayData().size(); ++i) {
        args.push(cmd->getArrayData()[i]);
    }

    bool isWriteCmd = std::find(this->writeCommands.begin(), this->writeCommands.end(), cmdName) != this->writeCommands.end();
    if (isWriteCmd && !this->server->isMaster()) throw CommandError("set", "Write operations can only be done by master server\n");
    if (isWriteCmd) {
        this->server->propagate(cmd->toRespString());
    }
    
    if (this->functionMap.find(cmdName) == this->functionMap.end()) throw CommandError("_", "command Name '" + cmdName + "' does not exist.");
    std::string response = this->functionMap[cmdName](this->server, this->curConn, args);

    return response;
}

void CommandManager::tryProcess(Conn* conn) {
    if (conn->processMem != nullptr) {
        if (conn->processMem->getType() == DataType::ARRAY) { // is a commmand
            try {
                this->curConn = conn;
                std::string response = this->runCommand(conn->processMem);
                tryAddResponse(conn, response);
            } catch (CommandError& e) {
                std::cerr << e.what() << "\n";
                this->curConn = nullptr;
                tryAddResponse(conn, "$-1\r\n");
            }
        }

        delete conn->processMem;
        conn->processMem = nullptr;
    }
}