#include "utils.h"

#pragma once

class Data {
private:
    int type = DataType::NONE;
    bool isImportant = false;

    std::vector<Data*> arrayData;
    std::string stringData;

public:
    Data() {};

    Data(std::string _strData);

    Data(std::vector<Data*> _arrData);

    Data(std::vector<std::string> _arrData);

    ~Data();

    std::string toRespString(bool readable = false);

    int getType() {return this->type;}
    std::vector<Data*> getArrayData() {return this->arrayData;}
    std::string getStringData() {return this->stringData;}
};