#include "Server.h"

Server::Server() {
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (this->fd < 0) {
      std::cerr << "Failed to create server socket\n";
      this->state = STATE_END;
  }
};

Server::~Server() {
  std::cerr << "Closing server...\n";
  for (const auto &[conn_fd, conn]: this->conns) {
    delete conn;
  }
}

void Server::acceptNewConn() {
  struct sockaddr_in client_addr; // for the single client
  int client_addr_len = sizeof(client_addr);
  
  std::cerr << "Waiting for a client to connect...\n";
  
  int client_fd = accept(this->getFd(), (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  if (client_fd < 0) {
    std::cerr << "Unable to connect client\n";
    return;
  }

  if (fd_set_nb(client_fd) < 0) {
    std::cerr << "Unable to set client connection to non-blocking mode\n";
    close(client_fd);
    return;
  }

  this->conns.insert({client_fd, new Conn(client_fd)});
  std::cerr << "Conn@" << client_fd << ": connected\n";
}

void Server::config(int argc, char ** argv) {
  if (!this->isRunning()) return;

  size_t ptr = 1;
  try {
    while (ptr < argc) {
      std::string opt(argv[ptr]);
      if (opt == "--port") {
        if (ptr + 1 == argc) throw std::runtime_error("Expected 1 argument for option '--port'. got nothing.");
        this->port = std::atoi(argv[ptr + 1]);
        ptr += 2;
        continue;
      }
      throw std::runtime_error("Unknown option '" + opt + "'");
    }
  } catch (std::exception& e){
    std::cerr << "error while parsing arguments => " << e.what() << "\n";
    this->closeServer(); return;
  }
  

  int reuse = 1;
  if (setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    this->closeServer(); return;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(this->port);
  
  if (bind(this->fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 6379\n";
    this->closeServer(); return;
  }

  int connection_backlog = 5;
  if (listen(this->fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    this->closeServer(); return;
  }

  if (fd_set_nb(this->fd) < 0) {
    std::cerr << "Failed to set server to non-blocking mode\n";
    this->closeServer(); return;
  }

  std::cerr << "Server online\n";
}

void Server::updateStatus() {
  this->last_poll_time = getNow();

  std::vector<pollfd> polls;
  std::stack<int> deadConns;

  struct pollfd pfd = {this->fd, POLLIN, 0};
  polls.push_back(pfd);

  for (const auto& [conn_fd, conn] : this->conns) {
      if (conn->state == STATE_END) {deadConns.push(conn_fd); continue;}

      auto op = POLLERR | POLLHUP;
      if (conn->state == STATE_READ) op |= POLLIN;
      if (conn->state == STATE_WRITE) op |= POLLOUT;

      polls.push_back(pollfd{conn_fd, (short) op, 0});
  }

  // deletes dead connections
  while (!deadConns.empty()) {
    this->deleteConnAt(deadConns.top());
    deadConns.pop();
  }

  int ready = poll(polls.data(), (nfds_t)polls.size(), 1000);
  if (ready < 0) {
    std::cerr << "Polling error\n";
    this->closeServer();
  }

  this->last_poll = (ready) ? polls[0].revents : 0;
  for (int i = 1; i < polls.size(); ++i) {
    this->conns[polls[i].fd]->last_poll = (ready) ? polls[i].revents : 0;
  }
}

void Server::setData(std::string key, Data* val) {
  if (key.length() == 0) return;
  val->setToImportant(); // so it does not get deleted

  this->database.insert({key, val});
}

Data* Server::getData(std::string key) {
  if (this->database.find(key) == this->database.end()) return nullptr;

  return this->database.at(key);
}

int Server::delData(std::string key) {
  auto data = this->getData(key);
  if (data == nullptr) return 0;

  this->database.erase(key);
  delete data;
  return 1;
}
