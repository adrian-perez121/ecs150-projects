#include <iostream>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>

int main(int argc, char* argv[]) {

  // start at one because the first argument is always the name of the program
  for (int i = 1; i < argc; i ++) {
    const char *file_name = argv[i];
    int f = open(file_name, O_RDONLY);
    

    if (f < 0) { // the read was not successful
      const char *err_msg = "wcat: cannot open file\n";
      write(STDOUT_FILENO, err_msg, strlen(err_msg));
      exit(1);
    }
    
    size_t buff_size = 10;
    char *buf = (char *) calloc(sizeof(char), buff_size);

    int bytes_read = 0;
    while ((bytes_read = read(f, buf, buff_size)) > 0){
      write(STDOUT_FILENO, buf, bytes_read);
    }


    close(f);
  }

  return 0;
}