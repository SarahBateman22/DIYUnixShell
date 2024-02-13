#include "shelpers.hpp"

using namespace std;

////////////////////////////////////////////////////////////////////////
// Example test commands you can try:
//
// ls
// ls | nl
// cd [dir]
// cat < shelpers.cpp
// cat < shelpers.cpp | nl
// cat shelpers.cpp | nl
// cat shelpers.cpp | nl | head -50 | tail -10
// cat shelpers.cpp | nl | head -50 | tail -10 > ten_lines.txt 
// 

////////////////////////////////////////////////////////////////////////

bool splitOnSymbol(vector<string> & words, int i, char c )
{
   if( words[i].size() < 2 ){
      return false;
   }
   int pos;
   if( (pos = words[i].find(c)) != string::npos ){
      if( pos == 0 ){
         // Starts with symbol.
         words.insert( words.begin() + i + 1, words[i].substr(1, words[i].size() -1) );
         words[i] = words[i].substr( 0, 1 );
      }
      else {
          // Symbol in middle or end.
          words.insert( words.begin() + i + 1, string{c} );
          string after = words[i].substr( pos + 1, words[i].size() - pos - 1 );
          if( !after.empty() ){
              words.insert( words.begin() + i + 2, after );
          }
          words[i] = words[i].substr( 0, pos );
      }
      return true;
   }
   else {
      return false;
   }
}

////////////////////////////////////////////////////////////////////////
// You do not need to modify tokenize().  

vector<string> tokenize( const string& s )
{
   vector<string> ret;
   int pos = 0;
   int space;

   // Split on spaces:

   while( (space = s.find(' ', pos) ) != string::npos ){
      string word = s.substr( pos, space - pos );
      if( !word.empty() ){
         ret.push_back( word );
      }
      pos = space + 1;
   }

   string lastWord = s.substr( pos, s.size() - pos );

   if( !lastWord.empty() ){
      ret.push_back( lastWord );
   }

   for( int i = 0; i < ret.size(); ++i ) {
      for( char c : {'&', '<', '>', '|'} ) {
         if( splitOnSymbol( ret, i, c ) ){
            --i;
            break;
         }
      }
   }
   return ret;
}

////////////////////////////////////////////////////////////////////////

ostream& operator<<( ostream& outs, const Command& c )
{
   outs << c.execName << " [argv: ";
   for( const auto & arg : c.argv ){
      if( arg ) {
         outs << arg << ' ';
      }
      else {
         outs << "NULL ";
      }
   }
   outs << "] -- FD, in: " << c.inputFd << ", out: " << c.outputFd << " "
        << (c.background ? "(background)" : "(foreground)");
   return outs;
}

////////////////////////////////////////////////////////////////////////
//
// getCommands()
//
// Parses a vector of command line tokens and places them into (as appropriate)
// separate Command structures.
//
// Returns an empty vector if the command line (tokens) is invalid.
//

vector<Command> getCommands( const vector<string> & tokens )
{
   vector<Command> commands( count( tokens.begin(), tokens.end(), "|") + 1 ); // 1 + num |'s commands

   int first = 0;
   int last = find( tokens.begin(), tokens.end(), "|" ) - tokens.begin();

   bool error = false;

   for( int cmdNumber = 0; cmdNumber < commands.size(); ++cmdNumber ){
      const string & token = tokens[ first ];

      if( token == "&" || token == "<" || token == ">" || token == "|" ) {
         error = true;
         break;
      }

      // Get reference to current Command struct.
      Command & command = commands[ cmdNumber ];
      command.execName = token;

      // Must _copy_ the token's string (otherwise, if token goes out of scope (anywhere)
      // this pointer would become bad

      command.argv.push_back( strdup( token.c_str() ) );

      command.inputFd  = STDIN_FILENO;
      command.outputFd = STDOUT_FILENO;

      command.background = false;
	
      for( int j = first + 1; j < last; ++j ) {

         if( tokens[j] == ">" || tokens[j] == "<" ) {
             //the argument after the redirection should be the filename
             std::string filename;
             //make sure the filename exists
             if(j+1 < tokens.size()){
                 filename = tokens[j + 1];
             }
             //otherwise just give it a generic name
             else{
                 filename = "file.txt";
             }

             if (tokens[j] == ">") {
                 //open an output file for outputFD
                 command.outputFd = open(filename.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0644);
                 //handle an error in opening the file
                 if (command.outputFd == -1) {
                     std::perror("opening outputFD failed.\n");
                     error = true;
                     break;
                 }
                 //skip the filename
                 j++;

             } else if (tokens[j] == "<") {
                 //open inputFD
                 command.inputFd = open(filename.c_str(), O_RDONLY);
                 if (command.inputFd == -1) {
                     //handle an error in opening the file
                     std::perror("opening inputFD failed.\n");
                     error = true;
                     break;
                 }
                 //skip the filename
                 j++;
             }
         } //end < and > if statement section
         else if( tokens[j] == "&" ){
            command.background = true;
         }
         else {
            // Otherwise this is a normal command line argument! Add to argv.
            command.argv.push_back( tokens[j].c_str() );
         }
      }

      if( !error ) {

         if( cmdNumber > 0 ){
            // There are multiple commands.  Open a pipe and
            // connect the ends to the fd's for the commands!
            int pipeFD[2];
            //readFD is pipeFD[0]
            //writeFD is pipeFD[1]

            //open a pipe and handle errors when creating it
            if(pipe(pipeFD) == -1){
                std::perror("opening pipe failure.\n");
                exit(1);
            }

            //write the output of the previous command (command - 1) to the writeFD
            commands[cmdNumber - 1].outputFd = pipeFD[1];
            //read the readFD as the input for the current command
            command.inputFd = pipeFD[0];

         }

         // Exec wants argv to have a nullptr at the end!
         command.argv.push_back( nullptr );

         // Find the next pipe character
         first = last + 1;

         if( first < tokens.size() ){
            last = find( tokens.begin() + first, tokens.end(), "|" ) - tokens.begin();
         }
      } // end if !error
   } // end for( cmdNumber = 0 to commands.size )

   if( error ){
       //need to go through every command in the vector of commands
       for (Command& command : commands) {
           //close anything that isn't the default
           if (command.inputFd != STDIN_FILENO) {
               close(command.inputFd);
           }
           if (command.outputFd != STDOUT_FILENO) {
               close(command.outputFd);
           }
           //free memory allocated in argv
           for (const char* arg : command.argv) {
               //when you hit the null pointer you know the list is finished
               if (arg != nullptr) {
                   free(const_cast<char*>(arg));
               }
           }
           //clear the vector
           command.argv.clear();
       }

       //send a perror message and return empty commands vector
       std::perror("Error occurred while processing commands. Resources have been cleaned up.\n");
       return commands;
   }
   //return good commands
   return commands;

} // end getCommands()
