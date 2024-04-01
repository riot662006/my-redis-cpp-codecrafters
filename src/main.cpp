#include "./resources/ServerManager.h"

int main(int argc, char **argv) {
    ServerManager redisServer = ServerManager();
    redisServer.runServer(argc, argv);
}