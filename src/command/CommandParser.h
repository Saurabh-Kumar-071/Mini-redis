#pragma once

#include<string>
#include<vector>
using namespace std;

struct ParsedCommand{
     vector<string>arguments;
};

class CommandParser{
  public:
    ParsedCommand parse( const string &input); // old parser for plain text
    ParsedCommand parseRESP(const string& input); // RESP PARSER
};
