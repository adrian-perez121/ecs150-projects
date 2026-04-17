#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>

using arg_vector = std::vector<std::string>;
char ERR_MSG[30] = "An error has occurred\n";
char PROMPT[7] = "wish> ";

// Outputs the error message to standard error and calls exit(1)
// Once this is run, the program is done.
void handle_error(bool terminating = false) {
  write(STDERR_FILENO, ERR_MSG, 30);
  if (terminating) {
    exit(1);
  }
}

// Returns what the user wrote into standard input
std::string get_user_input() {
  std::string input;
  write(STDOUT_FILENO, PROMPT, 7);
  std::getline(std::cin, input);

  return input;
}

// A struct for storing the command, its arguments, and the output location
// and executing it when its needed. Actions are separated by '&'.
struct Action {
  std::string command;
  std::vector<std::string> args;
  char **primitive_args;
  int output_location;

  Action(std::string command, arg_vector args,
         int output_location = STDOUT_FILENO)
      : command(command), args(args), output_location(output_location) {
        primitive_args = (char **) malloc(sizeof(char *) * (args.size() + 1));

        // Putting our data into a char ** because that's what execv takes
        for (size_t i = 0; i < args.size(); i++) {
          primitive_args[i] = (char *) args.at(i).c_str();
        }

        primitive_args[args.size()] = NULL; // null terminated list
      }

  // Like the name says, this function executes the action and sets the correct output_location.
  // This works for both built in commands and regular commands.
  //
  // WARNING: After this function runs the current program has been replaced with the program that was 
  // set to execute unless the command was a built in command.
  void execute() {
    dup2(output_location, STDOUT_FILENO);
    dup2(output_location, STDERR_FILENO);

    if (command == "exit") {

      if (args.size() != 0) {
        handle_error();
      }

      exit(0);
    } else if (command == "cd") {

      if (args.size() == 0 || args.size() > 1 || chdir(args[0].c_str()) == -1) {
        handle_error();
      }

    }
  }

};

int main(int argc, char *argv[]) {

  if (argc > 2) { // At most the shell can take in one file
    handle_error();
  }
  // auto user_input = get_user_input();

  // std::stringstream ss(user_input);
  // std::string word;
  // while (ss >> word) {
  //   std::cout << word << std::endl;
  // }
  arg_vector args = {".."};
  std::string cmd = "cd";
  Action test(cmd, args);

  test.execute();

  return 0;
}