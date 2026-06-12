#pragma once

#include<string>
using namespace std;

struct ParsedCommand{
     string command;
     string key;
     string value;
};

class CommandParser{
  public:
    ParsedCommand parse( const string &input);
};
