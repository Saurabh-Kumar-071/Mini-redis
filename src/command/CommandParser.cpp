
#include "CommandParser.h"
#include<sstream>
#include<iostream>

using namespace std;

ParseResult CommandParser::parseRESP(const string& input){
    ParseResult result;

    if (input.empty()) return result;

    // ---------- Inline protocol ----------
    if (input[0] != '*'){
        size_t end = input.find("\r\n");
        if (end == string::npos)
            return result;

        stringstream ss(input.substr(0, end));
        string word;

        while (ss >> word)
            result.command.arguments.push_back(word);

        result.complete = !result.command.arguments.empty();
        result.bytesConsumed = end + 2;

        return result;
    }

    // ---------- RESP protocol ----------
    size_t pos = 0;
    if (input[pos] != '*') {
        return result;
    }

    size_t lineEnd = input.find("\r\n", pos);
    if (lineEnd == string::npos) {
        return result;
    }

    int count = 0;
    try {
        count = stoi(input.substr(pos + 1, lineEnd - pos - 1));
    } catch (...) {
        // Malformed array count; consume bad line and return protocol error
        result.command.arguments.push_back("__MALFORMED_PROTOCOL_ERROR__");
        result.complete = true;
        result.bytesConsumed = lineEnd + 2;
        return result;
    }

    if (count <= 0) {
        result.command.arguments.push_back("__MALFORMED_PROTOCOL_ERROR__");
        result.complete = true;
        result.bytesConsumed = lineEnd + 2;
        return result;
    }

    pos = lineEnd + 2;

    // Read arguments
    for (int i = 0; i < count; i++) {
        if (pos >= input.size()) {
            return result; // Need more data
        }

        if (input[pos] != '$') {
            size_t nextEnd = input.find("\r\n", pos);
            if (nextEnd != string::npos) {
                result.command.arguments.clear();
                result.command.arguments.push_back("__MALFORMED_PROTOCOL_ERROR__");
                result.complete = true;
                result.bytesConsumed = nextEnd + 2;
                return result;
            }
            return result;
        }

        size_t lengthEnd = input.find("\r\n", pos);
        if (lengthEnd == string::npos) {
            return result;
        }

        int length = 0;
        try {
            length = stoi(input.substr(pos + 1, lengthEnd - pos - 1));
        } catch (...) {
            result.command.arguments.clear();
            result.command.arguments.push_back("__MALFORMED_PROTOCOL_ERROR__");
            result.complete = true;
            result.bytesConsumed = lengthEnd + 2;
            return result;
        }

        if (length < 0) {
            // Null bulk string
            pos = lengthEnd + 2;
            result.command.arguments.push_back("");
            continue;
        }

        pos = lengthEnd + 2;

        if (pos + length + 2 > input.size()) {
            return result; // Need more data
        }

        string arg = input.substr(pos, length);
        result.command.arguments.push_back(arg);
        pos += length + 2;
    }

    result.complete = true;
    result.bytesConsumed = pos;
    return result;
}
