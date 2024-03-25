#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char **argv) {
  // Server socket initializer
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }


  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 6379\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  // at this point, the server is on

  struct sockaddr_in client_addr; // for the single client
  int client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect...\n";
  
  int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  std::cout << "Client connected\n"; // at this point, the client has connected

  char rbuf[1024] = {0}; // read buffer... to receive data from client

  while (true) {
    int n = read(client_fd, rbuf, 1024);

    if (n == 0) break;
    
    while (n > 0) {
      if (n >= 14) {
        if (memcmp(rbuf, "*1\r\n$4\r\nping\r\n", 14) == 0) { // checks if the buffer has the ping command
          write(client_fd, "+PONG\r\n", 7); // sends PONG response
          n -= 14;
        }
      }
    }
  }
  
  close(server_fd);

  return 0;
}
