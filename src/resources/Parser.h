#include "utils.h"
#include "Data.h"
#pragma once

class ParserError : public std::exception {
    std::string msg;
    int ancestors = 0;

public:
    ParserError(const std::string& _msg) : msg(_msg) {};
    ParserError(const ParserError& err);

    const char* what() const noexcept override;
};

class ParserEOF : public std::exception {};

class CommandParser {
private:
    Buffer* buf;
    int data_type = DataType::NONE;
    int state = STATE_READ;

    CommandParser** inner; // for DataType::ARRAY
    std::string bulkStr; // for DataType::BULK_STR

    int size = -1; // size for DataType::BULK_STR or ARRAY

    int foundEndRN(size_t ptr);
    void parseInt(int& out);

    void parseString(int n, std::string& out);

public:
    CommandParser(Buffer* _buf) : buf(_buf)  {};
    ~CommandParser() {this->reset();}

    void parse();
    bool isDone() {return this->state == STATE_END;}
    
    Data* toData();
    void reset();
};