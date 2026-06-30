
#include "CommandParser.h"
#include<sstream>
#include<iostream>

using namespace std;

ParseResult CommandParser::parseRESP(const string& input){
    ParseResult result;

    size_t pos = 0;
    // ---------- Inline protocol ----------
if (!input.empty() && input[0] != '*'){
    size_t end = input.find("\r\n");

    if (end == string::npos)
        return result;

    stringstream ss(input.substr(0, end));
    string word;

    while (ss >> word)
        result.command.arguments.push_back(word);

    result.complete = true;
    result.bytesConsumed = end + 2;

    return result;
}


 /*  RESP protocol*/
    // Read *<count>
   if (pos >= input.size() || input[pos] != '*'){
    return result;
}
  size_t lineEnd = input.find("\r\n", pos);

  if (lineEnd == string::npos){
    return result;
}

   int count = stoi(input.substr(pos + 1, lineEnd - pos - 1));
   pos = lineEnd + 2;

    // Read arguments
    for (int i = 0; i < count; i++) {

    if (pos >= input.size() || input[pos] != '$'){
    return result;
    }

    size_t lengthEnd = input.find("\r\n", pos);
    if (lengthEnd == string::npos){
    return result;
}

    int length = stoi( input.substr(pos + 1, lengthEnd - pos - 1));
    pos = lengthEnd + 2;

    if (pos + length + 2 > input.size()){
    return result;
}

  string arg = input.substr(pos, length);
  result.command.arguments.push_back(arg);
  pos += length + 2;
    }

    result.complete = true;
   result.bytesConsumed = pos;
    return result;
}
