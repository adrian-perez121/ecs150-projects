#include <fcntl.h>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>

using arg_vector = std::vector<std::string>;
using path_vector = std::vector<std::string>;
using pid_queue = std::queue<pid_t>;
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
  arg_vector args;
  path_vector &paths;
  pid_queue &pids;
  char **primitive_args;
  int output_location;

  Action(std::string command, arg_vector args, path_vector &paths,
         pid_queue &pids, int output_location = STDOUT_FILENO)
      : command(command), args(args), paths(paths), pids(pids),
        output_location(output_location) {
    primitive_args = (char **)malloc(sizeof(char *) * (args.size() + 2));

    primitive_args[0] = (char *)(this->command).c_str();
    primitive_args[args.size() + 1] = NULL; // null terminated list

    // Putting our data into a char ** because that's what execv takes
    for (size_t i = 1; i < args.size() + 1; i++) {
      primitive_args[i] = (char *)this->args.at(i - 1).c_str();
    }
  }

  // Frees the primitive args and closes any files that were left open 
  ~Action() { 
    if (primitive_args) {
      free(primitive_args);
    }

    if (output_location != STDOUT_FILENO) {
      close(output_location);
    }
  }

  // returns the path to the command if it exists, else it returns an empty
  // string
  std::string get_command_path(std::string cmd) {
    std::string cmd_path;
    for (auto path : paths) {
      cmd_path = path + "/" + cmd;
      if (access(cmd_path.c_str(), X_OK) == 0) {
        return cmd_path;
      }
    }

    return "";
  }

  // Like the name says, this function executes the action and sets the correct
  // output_location. This works for both built in commands and regular
  // commands.
  //
  // WARNING: After this function runs the current program has been replaced
  // with the program that was set to execute unless the command was a built in
  // command.
  void execute() {
    dup2(output_location, STDOUT_FILENO);
    dup2(output_location, STDERR_FILENO);
    pid_t pid;

    if (command == "exit") {

      if (args.size() != 0) {
        handle_error();
      }

      exit(0);
    } else if (command == "cd") {

      if (args.size() == 0 || args.size() > 1 || chdir(args[0].c_str()) == -1) {
        handle_error();
      }

    } else if (command == "path") {
      paths.clear();
      for (auto arg :
           args) { // each argument is a different path that we can search
        paths.push_back(arg);
      }
    } else { // We run a non built in command
      std::string cmd_path = get_command_path(command);

      if (cmd_path.empty()) {
        handle_error();
      } else {
        pid = fork();

        // So when we execute different commands they will run in their own
        // process UNLESS its a built in command
        if (pid == 0) {
          execv(cmd_path.c_str(), primitive_args);
        } else { // append the child to the parent
          pids.push(pid);
        }
      }
    }
  }

  // Takes in a string of data, presumably one without any '&' symbols and
  // determines what the command is, its arguments, and the output destination.
  // If a parsing error happens, then a shell error is raised. Else, an Action
  // is returned.
  //
  // Special case: if only white space is given, no error is given and a nullptr
  // is returned
  static std::unique_ptr<Action>
  ParseAction(std::string action_text, path_vector &paths, pid_queue &pids,
              int output_location = STDOUT_FILENO) {
    std::stringstream ss(action_text);

    // Word in this context can also mean arguments or '>'
    std::string word;
    std::string cmd;
    arg_vector args;
    size_t pos;

    bool command_found = false;
    bool redirection_found = false;
    bool redirection_file_found = false;
    // There is a possible edge case regarding quoted arguments
    while (ss >> word) {
      // if there is more redirection or files after a redirection, we throw a
      // shell error
      if (redirection_file_found && redirection_found) {

        if (output_location !=
            STDOUT_FILENO) { // if we did open a file then make sure we close it
          close(output_location);
        }
        handle_error();
        return nullptr;
      }

      // Conventionally, the first argument is the command, you want to execute
      if ((pos = word.find(">")) !=
          std::string::npos) { // when there is a '>' we are redirecting the
        // output to the file adjacent of the '>'
        if (redirection_found) {
          handle_error();
          return nullptr;
        }

        redirection_found = true;

        std::string left = word.substr(0, pos);
        std::string right = word.substr(pos + 1);

        if (!left.empty()) { 
           if (!command_found) {
            cmd = left;
            command_found = true;
          } else { // if its not a command then its an argument to a command
            args.push_back(left);
          }
        }

        if (!right.empty()) {
          output_location = open(right.c_str(), O_WRONLY | O_TRUNC);
          if (output_location < 0) {
            handle_error();
            return nullptr;
          }

          redirection_file_found = true;
        }

      } else if (!command_found) {
        cmd = word;
        command_found = true;

      } else if (redirection_found && !redirection_file_found) {
        int output_location = open(word.c_str(), O_WRONLY | O_TRUNC);

        if (output_location < 0) {
          handle_error();
          return nullptr;
        }

        redirection_file_found = true;
      } else { // if none of the above cases are satisfied then it is an
               // argument
        args.push_back(word);
      }
    }

    if (!command_found) { // I believe this is not an error case, but still
                          // return a nullptr
      return nullptr;
    }

    if (redirection_found &&
        !redirection_file_found) { // we were given no file to go to
      handle_error();
      return nullptr;
    }

    return std::unique_ptr<Action>(
        new Action(cmd, args, paths, pids, output_location));
  }
};

int main(int argc, char *argv[]) {
  path_vector paths({"/bin"});
  std::string user_input;
  std::unique_ptr<Action> action;
  pid_queue pids;

  if (argc > 2) { // At most the shell can take in one file
    handle_error(true);
  }

  if (argc == 1) { // we are running in shell mode
    while (true) { // change this to be while pids not empty
      while (!pids.empty()) {
        waitpid(pids.front(), NULL, 0);
        pids.pop();
      }

      user_input = get_user_input();
      action = Action::ParseAction(user_input, paths, pids);
      if (action != nullptr) {
        action->execute();
      }
    }
  }
  // Next Todos:
  // - Make a parser that breaks down a line into different actions based on the
  // & symbol
  // - On each side of the & there should actions

  return 0;
}