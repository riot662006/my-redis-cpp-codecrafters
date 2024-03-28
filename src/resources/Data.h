#include "utils.h"

#pragma once

class Data {
private:
    int type = DataType::NONE;
    bool isImportant = false;

    std::vector<Data*> arrayData;
    std::string stringData;
    
    bool canExpire = false;
    Timepoint expiryPoint;

public:
    Data() {};

    Data(std::string _strData);

    Data(std::vector<Data*> _arrData);

    Data(std::vector<std::string> _arrData);

    ~Data();

    std::string toRespString(bool readable = false);
    void setToImportant() {this->isImportant = true;}

    void addExpiry(Timepoint ex) {canExpire = true; expiryPoint = ex;}
    void addExpiry(Timepoint from, size_t milli) {canExpire = true; expiryPoint = from + Milliseconds(milli);}
    bool hasExpired() {return (canExpire) ? getNow() >= expiryPoint : false;}

    int getType() {return this->type;}
    std::vector<Data*> getArrayData() {return this->arrayData;}
    std::string getStringData() {return this->stringData;}
};