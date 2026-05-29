#include <iostream>
#include <string>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "Disk.h"
#include "LocalFileSystem.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 4) {
    cerr << argv[0] << ": diskImageFile src_file dst_inode" << endl;
    cerr << "For example:" << endl;
    cerr << "    $ " << argv[0] << " tests/disk_images/a.img dthread.cpp 3"
         << endl;
    return 1;
  }

  // Parse command line arguments

  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  string srcFile = string(argv[2]);
  int dstInode = stoi(argv[3]);

  int bytes_read;
  // The size of the buffer doesn't really matter here
  char buffer[UFS_BLOCK_SIZE];
  string data;

  int fd = open(srcFile.c_str(), O_RDONLY);
  while ((bytes_read = read(fd, buffer, UFS_BLOCK_SIZE)) > 0) {
    data.append(buffer, bytes_read);
  }

  disk->beginTransaction();

  if (fileSystem->write(dstInode, data.c_str(), data.size()) < 0) {
    cerr << "Could not write to dst_file" << endl;

    disk->rollback();
    delete fileSystem;
    delete disk;

    return 1;
  }

  disk->commit();

  delete fileSystem;
  delete disk;
  return 0;
}
