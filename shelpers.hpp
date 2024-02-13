#pragma once

#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>

//////////////////////////////////////////////////////////////////////////////////
// A struct to contain a command line program information.
//

struct Command {
  std::string execName; // The name of the executable.
  std::vector<const char*> argv; 
  int                      inputFd, outputFd;
  bool                     background;
};

std::vector<Command> getCommands( const std::vector<std::string> & tokens );

std::vector< std::string > tokenize( const std::string & command_line_string );

std::ostream& operator<<( std::ostream& outs, const Command& c );
