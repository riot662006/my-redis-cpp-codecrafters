#include "Data.h"

Data::Data(std::string _strData) {
    this->type = DataType::BULK_STR;
    this->stringData = _strData;
}

Data::Data(std::vector<Data*> _arrData) {
    this->type = DataType::ARRAY;
    for (auto data : _arrData) this->arrayData.push_back(data);
}

Data::Data(std::vector<std::string> _arrData) {
    this->type = DataType::ARRAY;
    for (auto data : _arrData) this->arrayData.push_back(new Data(data));
}

Data::~Data() {
    for (auto data : this->arrayData) {
        if (!data->isImportant) delete data;
    }
}

std::string Data::toRespString(bool readable) {
    std::string res;

    if (this->type == DataType::BULK_STR) {
        res = "$" + std::to_string(this->stringData.length()) + "\r\n" + this->stringData + "\r\n";
    } 
    else if (this->type == DataType::ARRAY) {
        res = "*" + std::to_string(this->arrayData.size()) + "\r\n";
        for (auto data : this->arrayData) {
            res += data->toRespString();
        }
    } else {
        res = "$-1\r\n";
    }

    if (readable) return to_readable(res);
    return res;
}