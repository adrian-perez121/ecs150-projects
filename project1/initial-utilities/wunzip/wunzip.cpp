#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  std::string err_msg;
  uint32_t appearances;
  char c;
  int byte_read = 1;

  if (argc < 2) {
    err_msg = "wunzip: file1 [file2 ...]\n";
    write(STDOUT_FILENO, err_msg.c_str(), err_msg.size());
    exit(1);
  }

  for (int i = 1; i < argc; i++) {
    int f = open(argv[i], O_RDONLY);

    while ((byte_read = read(f, &appearances, 4)) > 0) {
      read(f, &c, 1);
      for (int j = 0; j < (int)appearances; j++) {
        write(STDOUT_FILENO, &c, 1);
      }
    }

    close(f);
  }

  return 0;
}