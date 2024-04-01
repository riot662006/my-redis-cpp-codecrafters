#include "Commands.h"
#include "Server.h"

#pragma once

class ServerManager {
private:
    CommandManager* cmdManager;
    Server* server;

public:
    ServerManager();
    void runServer(int argc, char** argv);
};