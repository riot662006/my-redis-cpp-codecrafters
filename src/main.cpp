#include "./resources/Server.h"
#include "./resources/Commands.h"

int main(int argc, char **argv) {
    // Server socket initializer
    Server* server = new Server();
    serverConfig(server, argc, argv);

    CommandManager cmdManager = CommandManager(server);

    while (server->isRunning()) {
        server->updateStatus();

        for (auto& [conn_fd, conn] : server->conns) {
            if (conn->last_poll | POLLIN) {
                tryRead(conn);
            }

            if (conn->processMem != nullptr) {
                if (conn->processMem->getType() == DataType::ARRAY) { // is a commmand
                    try {
                        std::string response = cmdManager.runCommand(conn->processMem);

                        tryAddResponse(conn, response);
                    } catch (CommandError& e) {
                        std::cerr << e.what() << "\n";
                        tryAddResponse(conn, "$-1\r\n");
                    }
                }

                delete conn->processMem;
                conn->processMem = nullptr;
            } 

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