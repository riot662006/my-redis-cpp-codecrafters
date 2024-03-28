#include "./resources/Server.h"
#include "./resources/Commands.h"

int main(int argc, char **argv) {
    // Server socket initializer
    Server* server = new Server();
    server->config(argc, argv);

    CommandManager cmdManager = CommandManager(server);

    while (server->isRunning()) {
        server->updateStatus();

        for (auto& [conn_fd, conn] : server->conns) {
            if (conn->last_poll | POLLIN) {
                tryRead(conn);
            }

            cmdManager.tryProcess(conn);

            if (conn->last_poll | POLLOUT) {
                tryWrite(conn);
            }
        }

        // accept new connections if any
        if (server->last_poll) server->acceptNewConn();
    }

    delete server;
    return 0;
}