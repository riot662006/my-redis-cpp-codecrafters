#include "Parser.h"

ParserError::ParserError(const ParserError& err) {
    ++this->ancestors;
    this->msg = err.msg; 
};

const char* ParserError::what() const noexcept {
    static std::string fullMsg;
    fullMsg = "Error: parser (";
    fullMsg += std::to_string(ancestors) + ") => " + msg;

    return fullMsg.c_str();
};

int CommandParser::foundEndRN(size_t ptr) {
    if (this->buf->at(ptr) == '\r') {
        if (this->buf->at(ptr + 1) == '\n') return 1;
        return -1;
    }
    
    return 0;
}

void CommandParser::parseInt(int& out) {
    int ptr = 0;

    while (ptr < this->buf->size()) {
        int found = this->foundEndRN(ptr);

        if (found == 1) break;
        else if (found == -1) throw ParserError("Did not find ending '\\r\\n'");

        if ((this->buf->at(ptr) < '0') || ('9' < this->buf->at(ptr))) {
            std::string errMsg = "Expected digits. got '";
            errMsg.push_back(this->buf->at(ptr));
            errMsg.append("'\n");
            throw ParserError(errMsg);
        }
        ++ptr;
    }

    if (this->foundEndRN(ptr) != 1) throw ParserError("Did not find ending '\\r\\n'");
    out = std::stoi(this->buf->getString(ptr));
    this->buf->clearData(ptr + 2);
}

void CommandParser::parseString(int n, std::string& out) {
    int found = this->foundEndRN(n);
    if (found == 1) {
        out = this->buf->getString(n);
        this->buf->clearData(n + 2);
        return;
    }

    throw ParserError("string data should end in '\\r\\n'");
}

void CommandParser::parse() {
    if (this->state == STATE_END) throw ParserEOF();
    if (this->buf->isEmpty()) throw ParserEOF();
    if (data_type == DataType::NONE) {
        char id = this->buf->at(0);
        auto it = DataType::identifiers.find(id);

        try {
            this->data_type = DataType::identifiers.at(id);
        } catch (const std::out_of_range& e) {
            throw ParserError("Could not find type identifier");
        }

        this->buf->pop();
        return;
    }

    switch (this->data_type){
        case (DataType::ARRAY):
            if (this->size == -1) {
                
                this->parseInt(this->size);
                this->inner = new CommandParser*[this->size];
                for (size_t i = 0; i < this->size; ++i) {
                    this->inner[i] = nullptr;
                }
                
                return;
            }

            try {
                for (size_t i = 0; i < this->size; ++i) {
                    if (this->inner[i] == nullptr) {
                        this->inner[i] = new CommandParser(this->buf);
                    }
                    if (this->inner[i]->isDone()) continue;
                    this->inner[i]->parse();
                    if (i + 1 == this->size && this->inner[i]->isDone()) {
                        this->state = STATE_END;
                    }
                    return;
                }
            } catch (ParserError& e) {
                throw ParserError(e);
            }
        
        case (DataType::BULK_STR):
            if (this->size == -1) {
                this->parseInt(this->size);
                return;
            }

            this->parseString(this->size, this->bulkStr);
            this->state = STATE_END;
            return;
    }

    throw ParserError("Still working here!\n");
}

Data* CommandParser::toData() {
    if (!this->isDone()) return new Data();
    
    if (this->data_type == DataType::BULK_STR) return new Data(this->bulkStr);
    if (this->data_type == DataType::ARRAY) {
        std::vector<Data*> innerData;

        for (size_t i = 0; i < this->size; ++i) {
            innerData.push_back(this->inner[i]->toData());
        }

        return new Data(innerData);
    }

    return new Data();
}

void CommandParser::reset() {
    if (this->data_type == DataType::ARRAY) {
        for (int i = 0; i < this->size; ++i) {
            delete this->inner[i];
        }
    }

    this->data_type = DataType::NONE;
    this->state = STATE_READ;

    this->bulkStr = "";

    this->size = -1;
}