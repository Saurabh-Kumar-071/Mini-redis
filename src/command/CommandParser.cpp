
#include "CommandParser.h"
#include<sstream>

using namespace std;

ParsedCommand CommandParser::parse( const string &input){

      ParsedCommand result;
      stringstream ss(input);
      string token;

      while(ss >> token){
        result.arguments.push_back(token);
      }

  return result;
}
