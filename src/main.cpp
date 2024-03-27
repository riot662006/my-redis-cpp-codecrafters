#include "./resources/Server.h"
#include "./resources/Commands.h"

int main(int argc, char **argv) {
    // Server socket initializer
    Server* server = new Server();
    serverConfig(server, argc, argv);

    CommandManager cmdManager = CommandManager(server);
    
    std::vector<pollfd> polls;
    std::stack<int> deadConns;

    while (server->isRunning()) {
        polls.clear();

        // server fd, client fd_1, ... client fd_n
        struct pollfd pfd = {server->getFd(), POLLIN, 0};
        polls.push_back(pfd);

        for (const auto& [conn_fd, conn] : server->conns) {
            if (conn->state == STATE_END) {deadConns.push(conn_fd); continue;}

            auto op = POLLERR | POLLHUP;
            if (conn->state == STATE_READ) op |= POLLIN;
            if (conn->state == STATE_WRITE) op |= POLLOUT;

            polls.push_back(pollfd{conn_fd, (short) op, 0});
        }

        // deletes dead connections
        while (!deadConns.empty()) {
            server->deleteConnAt(deadConns.top());
            deadConns.pop();
        }

        int ready = poll(polls.data(), (nfds_t)polls.size(), 1000);
        if (ready < 0) {
            std::cerr << "Polling error\n";
            server->closeServer();
        }

        if (ready == 0) continue;

        for (int i = 1; i < polls.size(); ++i) {
            Conn* conn = server->getConn(polls[i].fd);

            if (polls[i].revents | POLLIN) {
                tryRead(conn);
            }

            if (conn->processMem != nullptr) {
                if (conn->processMem->getType() == DataType::ARRAY) { // is a commmand
                    try {
                        std::string response = cmdManager.runCommand(conn->processMem);
                        delete conn->processMem;
                        conn->processMem = nullptr;

                        tryAddResponse(conn, response);
                    } catch (CommandError& e) {
                        std::cerr << e.what() << "\n";
                    }
                }
            } 

            if (polls[i].revents | POLLOUT) {
                tryWrite(conn);
            }
        }

        // accept new connections if any
        if (polls[0].revents) server->acceptNewConn();
    }

    delete server;
    return 0;
}