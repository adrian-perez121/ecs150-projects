#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

// Takes in a line and a pattern, the return will be boolean dictating if the
// line passed in contains the pattern passed in.
bool line_contains_pattern(std::string line, std::string pattern) {
  if (pattern.size() > line.size()) {
    return false;
  }
  size_t k = 0; // To match with the pattern
  for (size_t i = 0;
       (i < line.length() - pattern.size()) && (k < pattern.size()); i++) {
    // brute force way of string matching

    k = 0;

    for (size_t j = i; k < pattern.size(); j++) {
      if (line[j] == pattern[k]) { // Matching
        k++;
      } else { // when there is not a match
        break;
      }
    }
  }
  return k == pattern.size();
}

// read from the file descriptor and checks for the pattern within the file
// if the pattern is found it will be printed to standard output
void perform_grep_on(int file_descriptor, std::string pattern) {
  std::string line; // storing in a string so i don't have to manage the
                    // buffer size myself
  char c;
  int byte_read = 1; // 1 is a dummy here

  while (byte_read > 0) { // the line reading loop
    line = "";

    while ((byte_read = read(file_descriptor, &c, 1)) > 0) {
      line += c;

      if (c == '\n') {
        break;
      }
    }

    if (line_contains_pattern(line, pattern)) { // The pattern was found
      write(STDOUT_FILENO, line.c_str(), line.size());
    }
  }
}

int main(int argc, char *argv[]) {
  std::string err_msg;
  int f = STDIN_FILENO; // by default we will use standard input
  std::string pattern;
  const char *file_name;

  if (argc < 2) {
    err_msg = "wgrep: searchterm [file ...]\n";
    write(STDOUT_FILENO, err_msg.c_str(), err_msg.size());
    exit(1);
  }

  pattern = std::string(argv[1]);
  if (argc == 2) { // then no files were specified
    perform_grep_on(f, pattern);
  } else { // we must go through all the files that we specified
    file_name = "README.md";

    for (int i = 2; i < argc; i++) {
      file_name = argv[i];
      f = open(file_name, O_RDONLY);

      if (f < 0) {
        err_msg = "wgrep: cannot open file\n";
        write(STDOUT_FILENO, err_msg.c_str(), err_msg.size());
        exit(1);
      }

      perform_grep_on(f, pattern);

      close(f);
    }
  }

  return 0;
}