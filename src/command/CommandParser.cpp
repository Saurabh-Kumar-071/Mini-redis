
#include "CommandParser.h"
#include<sstream>
#include<iostream>

using namespace std;

ParsedCommand CommandParser::parseRESP(const string& input){
    ParsedCommand result;

    stringstream ss(input);
    string line;

    // Read *<count>
    getline(ss, line);

    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    int count = stoi(line.substr(1));

    // Read arguments
    for (int i = 0; i < count; i++) {
        // Skip $<length>
        getline(ss, line);

        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        // Read actual argument
        getline(ss, line);

        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        result.arguments.push_back(line);
    }

    return result;
}
