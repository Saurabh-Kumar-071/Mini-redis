#include "RESPEncoder.h"

using namespace std;

string RESPEncoder::simpleString(const string& str){
    return "+" + str + "\r\n";
}

string RESPEncoder::bulkString(const string& str){
    return "$" + to_string(str.size()) + "\r\n"
            + str + "\r\n";
}

string RESPEncoder::integer(long long value){
    return ":" + to_string(value) + "\r\n";
}

string RESPEncoder::error(const string& str){
    return "-" + str + "\r\n";
}

string RESPEncoder::nullBulkString(){
    return "$-1\r\n";
}

string RESPEncoder::array(const vector<string>& arr){
    string resp = "*" + to_string(arr.size()) + "\r\n";

    for (const auto& item : arr){
        if (item == "Not-found") {
            resp += "$-1\r\n";
        }
        else{
            resp += "$" + to_string(item.size()) + "\r\n";
            resp += item + "\r\n";
        }
    }
    return resp;
}
