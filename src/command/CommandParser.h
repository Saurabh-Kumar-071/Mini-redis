#pragma once

#include<string>
#include<vector>
using namespace std;

struct ParsedCommand{
     vector<string>arguments;
};

struct ParseResult{            //
    ParsedCommand command;
    size_t bytesConsumed = 0;
    bool complete = false;
};

class CommandParser{
  public:
   ParseResult parseRESP(const string& input); // RESP PARSER    it is like you receive three command then run one by one (consume)
};
