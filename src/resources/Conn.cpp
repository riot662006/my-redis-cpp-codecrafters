#include "Conn.h"

void tryRead(Conn* conn) {
    if (conn->state == STATE_END) return;
    
    int n;
    char* tempBuffer = new char[MAX_BUF_SIZE - conn->rbuf->size()];

    do {
        n = recv(conn->fd, tempBuffer, MAX_BUF_SIZE - conn->rbuf->size(), 0);
    } while (n < 0 && errno == EINTR); // makes sure to get data if any
    
    if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) { // error
            std::cerr << "Conn@" << conn->fd << ": read() error\n";
            conn->state = STATE_END;
        }
    } else if (n == 0) { // socket connection is closed
        std::cerr << "Conn@" << conn->fd << ": EoF\n";
        conn->state = STATE_END;
    } else {
        conn->rbuf->addToBuffer(tempBuffer, n);
    }

    if (conn->state == STATE_END) return;

    try {
        while (true){
            conn->parser->parse(); // parses until useful info is completely gotten and the parsr closes / an error occurs
        }
    } catch (ParserError& e){
        std::cerr << e.what() << '\n';
        conn->state = STATE_END;
    } catch (ParserEOF& e) { // if an eof, checks if parser got information
        if (!conn->parser->isDone()) return;

        conn->processMem = conn->parser->toData();
        conn->parser->reset();
    }
} 

void tryWrite(Conn* conn) {
    if (conn->state == STATE_END) return;
    if (conn->writeQueue.empty()) return;

    int n;
    std::string response = conn->writeQueue.front();
    conn->writeQueue.pop_front();

    if (response.length() > 0) {
        do {
            n = send(conn->fd, response.data(), response.length(), 0);
        } while (n < 0 && errno == EINTR);

        if (n < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cerr << "Conn@" << conn->fd << ": write() error\n";
                conn->state = STATE_END;
            }
        } else if (n == 0) {
            std::cerr << "Conn@" << conn->fd << ": write() error\n";
            conn->state = STATE_END;
        } else {
            if (n < response.length()) {
                conn->writeQueue.push_front(response.substr(n));
            }
        }
    } else {
        std::cerr << "Conn@" << conn->fd << ": write() error\n";
        conn->state = STATE_END;
    }

    if (conn->state == STATE_WRITE && conn->writeQueue.empty()) conn->state = STATE_READ;
}

void tryAddResponse(Conn* conn, std::string response) {
    if (response.length() > 0) {
        conn->writeQueue.push_back(response);
        conn->state = STATE_WRITE;
    }
}