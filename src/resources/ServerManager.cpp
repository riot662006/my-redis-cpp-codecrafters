#include "ServerManager.h"

ServerManager::ServerManager() {
    this->server = new Server();
    this->cmdManager = new CommandManager(server);
}

void ServerManager::runServer(int argc, char** argv) {
    this->server->config(argc, argv);

    while (this->server->isRunning()) {
        this->server->updateStatus();

        if (!this->server->isMaster()) {
            Conn* conn = this->server->getMasterConn();

            if (conn->last_poll | POLLIN) {
                tryRead(conn);
            }

            if (conn->processMem != nullptr) {
                this->cmdManager->tryProcess(conn);
                continue;
            }
        }

        for (auto& [conn_fd, conn] : this->server->conns) {
            if (conn->last_poll | POLLIN) {
                tryRead(conn);
            }

            this->cmdManager->tryProcess(conn);

            if (conn->last_poll | POLLOUT) {
                tryWrite(conn);
            }
        }

        // accept new connections if any
        if (this->server->last_poll) this->server->acceptNewConn();
    }

    delete this->cmdManager;
    delete this->server;
}
