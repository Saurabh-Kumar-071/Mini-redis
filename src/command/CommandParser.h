#pragma once

#include<string>
#include<vector>
using namespace std;

struct ParsedCommand{
     vector<string>arguments;
};

class CommandParser{
  public:
    ParsedCommand parse( const string &input);
};
