#include <iostream>
#include <readline/readline.h>
#include <readline/history.h>
#include "shelpers.hpp"


/*NOTE ON USING EXECVP
 *
 * found on https://www.digitalocean.com/community/tutorials/execvp-function-c-plus-plus
 *
 * input params are execvp(const char* command, char* argv[]) and it returns an int which is the status code
 * the first param is the name of the command or file to be run
 * the second param is an array of arguments to command
 * the argv array needs to end with a nullpointer
 *
 * EXAMPLE:
 *
 * char* argument_list[] = {"ls", "-l", NULL}; // NULL terminated array of char* strings
 * //Ok! Will execute the command "ls -l"
 * execvp("ls", argument_list);
 *
 * */

void handleInput(char* &input, std::string &userInput);
void executeCommand(const std::vector<Command> &commands);
void changeDirectory(const std::vector<std::string> &tokens);
void cleanup(std::vector<int> &pipefds, const std::vector<Command> &commands);

int main() {
    std::string userInput;
    std::vector<std::string> tokens;
    std::vector<Command> commands;

    //readline syntax: char *line = readline ("Enter a line: "); and reads after the end quotation
    //while loop that continues to read until it hits a null pointer (forever as long as you don't abort the process
    //or type "exit")
    char* input;
    while ((input = readline("$ ")) != nullptr) {
        handleInput(input, userInput);

        // use the tokenize function in shelpers.cpp to tokenize the input
        tokens = tokenize(userInput);

        //put the tokens as a param into getCommands to extract the commands from the input
        commands = getCommands(tokens);

        if (!tokens.empty()) {
            //if the input is exit then break out of the loop
            if (tokens[0] == "exit") {
                break;
            }
            if (tokens[0] == "cd") {
                changeDirectory(tokens);
                continue;
            }
        }
        executeCommand(commands);
    }
    return 0;
}

void handleInput(char* &input, std::string &userInput) {
    //turn char* to string to manipulate further
    userInput = std::string(input);
    //as long as the input is not empty add it to the history
    if (!userInput.empty()) {
        //add_history is declared in readline.h (imported)
        add_history(input);
    }
    //free the input (we have it in userInput now)
    free(input);
}

void changeDirectory(const std::vector<std::string> &tokens) {
    std::string directory;
    //if there aren't two arguments then there's no file name to go to, should go to home
    if (tokens.size() < 2) {
        directory = getenv("HOME");
    }
        //otherwise assign the directory to the next input string
    else {
        directory = tokens[1];
    }
    //attempt to change the directory
    int status = chdir(directory.c_str());

    //handle any error codes returned
    if (status != 0) {
        perror("change directory failure\n");
    }
}

void executeCommand(const std::vector<Command>& commands) {
    if (commands.empty()) return;
    std::vector<int> pipefds;
    //go through every command
    for (size_t i = 0; i < commands.size(); i++) {
        if (i < commands.size() - 1) {
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                std::perror("pipe failure\n");
                exit(1);
            }

            //pipeFD[0] is the READ end
            pipefds.push_back(pipefd[0]);
            //pipeFD[1] is the WRITE end
            pipefds.push_back(pipefd[1]);
        }

        //create a fork for the parent and child processes
        pid_t pid = fork();
        //handle the child process (pid is 0)
        if (pid == 0) {
            //if the input file descriptor contains something other than the default filler
            if (i == 0 && commands[i].inputFd != STDIN_FILENO) {
                //duplicate the inputFD info
                if (dup2(commands[i].inputFd, STDIN_FILENO) == -1) {
                    std::perror("dup2 error\n");
                    exit(1);
                }
                //close the fd
                close(commands[i].inputFd);
            }

            //if the output file descriptor has info
            if (i == commands.size() - 1 && commands[i].outputFd != STDOUT_FILENO) {
                if (dup2(commands[i].outputFd, STDOUT_FILENO) == -1) {
                    std::perror("dup2 error\n");
                    exit(1);
                }
                //close the fd
                close(commands[i].outputFd);
            }

            //if the command is NOT the first command, get the info from the previous pipe
            if (i > 0) {
                dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
            }

            //if the command is NOT the last one, write info out to the next pipe
            if (i < commands.size() - 1) {
                dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
            }
            //close all the pipe file descriptors
            for (int fd: pipefds) {
                close(fd);
            }

            //see the long note above about execvp
            //need to convert to the right types to pass in as params
            //array (vector) of char* for argv (second param)
            std::vector<char *> argv;
            for (auto &arg: commands[i].argv) {
                argv.push_back(const_cast<char *>(arg));
            }
            argv.push_back(nullptr);

            //execute the command
            if (execvp(commands[i].execName.c_str(), argv.data()) == -1) {
                std::perror("Command failed to execute.\n");
                //child needs to exit if it fails
                exit(EXIT_FAILURE);
            }
        }

        //if the pid is less than 0 there was an error in creating the fork
        else if (pid < 0) {
            std::perror("fork failure");
            exit(1);
        }
    }
    cleanup(pipefds, commands);
}



void cleanup(std::vector<int> &pipefds, const std::vector<Command> &commands) {
    //close the pipefds in the parent
    for (int fd: pipefds) {
        close(fd);
    }
    //handle parent process (pid is above 0)
    for (size_t i = 0; i < commands.size(); ++i) {
        //wait for the child process to finish
        wait(nullptr);
    }
}