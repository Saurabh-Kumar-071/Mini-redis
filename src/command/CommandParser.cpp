
#include "CommandParser.h"
#include<sstream>

using namespace std;

ParsedCommand CommandParser::parse( const string &input){

      ParsedCommand result;
      stringstream ss(input);
      
       ss>>result.command;
       ss>>result.key;
       ss>>result.value;
  return result;
}
