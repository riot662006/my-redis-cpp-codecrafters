#include "Server.h"

const char* HandshakeError::what() const noexcept {
    static std::string fullMsg;
    fullMsg = "Error: handshake error @stage " + std::to_string(this->stage) + " => " + this->msg;

    return fullMsg.c_str();
}

Master::Master(std::string _host, int _port) {
  this->host = _host;
  this->port = _port;

  this->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (this->fd < 0) {
    std::cerr << "Failed to create master socket\n";
    this->state = STATE_END;
  }
};

void Master::handshake() {
  if (!this->isRunning()) return;

  char tempBuffer[1024] = {0};
  Buffer buffer = Buffer();

  // stage 1
  std::cerr << "Handshake stage 1...";
  std::string ping{"*1\r\n$4\r\nping\r\n"};

  int n = send(this->fd, ping.c_str(), ping.size(), 0);
  if (n < ping.size()) throw HandshakeError(1, "Could not send ping");

  n = recv(this->fd, tempBuffer, 1024, 0);
  if (n <= 0) throw HandshakeError(1,"Did not receive feedback on ping command");

  buffer.addToBuffer(tempBuffer, n);
  if (buffer.getString(n) != "+PONG\r\n") throw HandshakeError(1, "Invalid response to ping command. Got '" + buffer.getString(n, true) + "'");
  buffer.clearData(n);
  std::cerr << "complete\n";

  // stage 2
  std::cerr << "Handshake stage 2...";
  std::string replconf1 = Data(std::vector<std::string>{"REPLCONF", "listening-port", std::to_string(this->server->port)}).toRespString();

  n = send(this->fd, replconf1.c_str(), replconf1.size(), 0);
  if (n < replconf1.size()) throw HandshakeError(2, "Could not send first replconf command");

  n = recv(this->fd, tempBuffer, 1024, 0);
  if (n <= 0) throw HandshakeError(1,"Did not receive feedback on replconf command");

  buffer.addToBuffer(tempBuffer, n);
  if (buffer.getString(n) != "+OK\r\n") throw HandshakeError(2, "Invalid response to replconf command. Got '" + buffer.getString(n, true) + "'");
  buffer.clearData(n);

  std::string replconf2 = Data(std::vector<std::string>{"REPLCONF", "capa", "psync2"}).toRespString();

  n = send(this->fd, replconf2.c_str(), replconf2.size(), 0);
  if (n < replconf2.size()) throw HandshakeError(2, "Could not send second replconf command");

  n = recv(this->fd, tempBuffer, 1024, 0);
  if (n <= 0) throw HandshakeError(1,"Did not receive feedback on replconf command");

  buffer.addToBuffer(tempBuffer, n);
  if (buffer.getString(n) != "+OK\r\n") throw HandshakeError(2, "Invalid response to replconf command. Got '" + buffer.getString(n, true) + "'");
  buffer.clearData(n);

  std::cerr << "complete\n";

  // stage 3
  std::cerr << "Handshake stage 3...";
  std::string psync = "*3\r\n$5\r\nPSYNC\r\n$1\r\n?\r\n$2\r\n-1\r\n";

  n = send(this->fd, psync.c_str(), psync.size(), 0);
  if (n < psync.size()) throw HandshakeError(3, "Could not send psync command");

  n = recv(this->fd, tempBuffer, 1024, 0);
  if (n <= 0) throw HandshakeError(3, "Did not receive feedback on psync command");

  buffer.addToBuffer(tempBuffer, n);

  HandshakeError commonFullResyncError = HandshakeError(3, "Error while trying to resync. Got '" + buffer.getString(n, true) + "'");

  if (buffer.getString(12) != "+FULLRESYNC ") throw commonFullResyncError;
  buffer.clearData(12);

  int loc = buffer.find(' ');
  if (loc == -1) throw commonFullResyncError;
  if (loc != 40) throw HandshakeError(3, "Replication ids are 40 characters. got " + std::to_string(loc));

  server->master_replid = buffer.getString(loc);
  buffer.clearData(41);

  loc = buffer.find('\r');
  if (loc == -1) throw commonFullResyncError;
  if (buffer.at(loc + 1) != '\n') throw commonFullResyncError;

  server->master_repl_offset = std::stoi(buffer.getString(loc));
  buffer.clearData(loc + 2);
  
  if (buffer.isEmpty()) {
    n = recv(this->fd, tempBuffer, 1024, 0);
    if (n <= 0) throw HandshakeError(3, "Did not receive the RDB file");

    buffer.addToBuffer(tempBuffer, n);
  }

  std::cerr << buffer.getString(n, true) << '\n'; // will enable parsing later
  std::cerr << "complete\n";
}

void Master::config(Server* _server) {
  if (!this->isRunning()) return;

  this->server = _server;
  
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  // Handling "localhost" specifically or ensuring host is an IP address
  std::string ip = (host == "localhost") ? "127.0.0.1" : host;
  if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
    std::cerr << "Invalid master address / master address not supported\n";
    this->closeConn(); return;
  }

  if (connect(this->fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    std::cerr << "Failed to connect to master server\n";
    this->closeConn(); return;
  }

  std::cerr << "Connected to master server at " << this->host << "::" << port << "\n";

  try {
    this->handshake();
  } catch (HandshakeError& e) {
    std::cerr << e.what() << '\n';
    this->closeConn();
  }

  if (fd_set_nb(this->fd) < 0) {
    std::cerr << "Unable to set client connection to non-blocking mode\n";
    this->closeConn();
    return;
  }

  this->conn = new Conn(this->fd);
}

Server::Server() {
  this->fd = socket(AF_INET, SOCK_STREAM, 0);
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

  if (this->master != nullptr) delete this->master;
  this->master = nullptr;
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
      if (opt == "--replicaof") {
        if (ptr + 2 >= argc) throw std::runtime_error("Expected 2 argument for option '--replicaof'. got nothing.");
        
        this->master = new Master(std::string(argv[ptr + 1]), std::atoi(argv[ptr + 2]));
        this->role = "slave";
        this->master->config(this);

        if (!this->master->isRunning()) {
          this->closeServer(); return;
        }

        ptr += 3;
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

  this->generateReplId();
  std::cerr << "Server online\n";
}

void Server::updateStatus() {
  this->last_poll_time = getNow();

  std::vector<pollfd> polls;

  std::stack<int> deadConns;

  struct pollfd pfd = {this->fd, POLLIN, 0};
  polls.push_back(pfd);

  if (this->master != nullptr) {
    polls.push_back(pollfd{this->master->fd, POLLIN | POLLERR | POLLHUP, 0});
  }

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

  if (this->master != nullptr) {
    this->master->conn->last_poll = (ready) ? polls[1].revents : 0;
  }

  for (int i = ((this->master == nullptr) ? 1 : 2) ; i < polls.size(); ++i) {
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

std::string Server::getReplInfo() {
  std::stringstream info;
  info << "role:" << this->role << "\n";
  info << "connected_slaves:" << this->connected_slaves << "\n";
  info << "master_replid:" << this->master_replid << "\n";
  info << "master_repl_offset:" << this->master_repl_offset << "\n";
  info << "second_repl_offset:" << this->second_repl_offset << "\n";
  info << "repl_backlog_active:" << this->repl_backlog_active << "\n";
  info << "repl_backlog_size:" << this->repl_backlog_size << "\n";
  info << "repl_backlog_first_byte_offset:" << this->repl_backlog_first_byte_offset << "\n";
  info << "repl_backlog_histlen:" << this->repl_backlog_histlen;

  return info.str();
}

void Server::propagate(std::string cmd) {
  for (auto &[conn_fd, conn] :this->conns) {
    if (conn->isSlave) conn->writeQueue.push_back(cmd);
  }
}

void Server::generateReplId() {
  const std::string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, characters.size() - 1);

  this->master_replid = "";
  for (int i = 0; i < 40; ++i) {
    this->master_replid += characters[dis(gen)];
  }
}