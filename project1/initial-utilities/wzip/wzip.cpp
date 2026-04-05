#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <string> 

// Writes the compressed information to standard outpout
void write_out_compressed(char c, uint32_t appearances) {
  write(STDOUT_FILENO, (void *) &appearances, sizeof(u_int32_t));
  write(STDOUT_FILENO, (void *) &c, 1);
}

int main(int argc, char *argv[]) {
  std::string err_msg;
  int buf_size = 1;
  char buf[1];

  // for the implementation
  uint32_t appearances = 1; // uint 32 is bytes big (4 x 8 = 32)
  char prev = EOF;
  char current;
  // if less than or equal to 0 then, there is no more of the file left to read
  int byte_read;
  // Enough arguments
  if (argc < 2) {
    err_msg = "wzip: file1 [file2 ...]\n";
    write(STDOUT_FILENO, err_msg.c_str(), err_msg.size());
    exit(1);
  }

  for (int i = 1; i < argc; i++) {
    int f = open(argv[i], O_RDONLY);
    while ((byte_read = read(f, buf, buf_size) > 0)) {  
      current = buf[0];
      if (prev == EOF) { // happens when starting off
        prev = current; 
        continue; 
      }

      if (current == prev) {
        appearances++;
      } else {
        // now we have to write it to the file unless its the very first character we have seen
        write_out_compressed(prev, appearances);

        prev = current;
        appearances = 1;
      } 
    }
    close(f);
  }

  write_out_compressed(current, appearances);

  return 0;
}