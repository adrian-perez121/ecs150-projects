#include <iostream>
#include <string>
#include <vector>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 4) {
    cerr << argv[0] << ": diskImageFile parentInode directory" << endl;
    cerr << "For example:" << endl;
    cerr << "    $ " << argv[0] << " a.img 0 a" << endl;
    return 1;
  }

  // Parse command line arguments

  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  int parentInode = stoi(argv[2]);
  string directory = string(argv[3]);

  disk->beginTransaction();

  // 0 is reserved for the root I believe
  if (fileSystem->create(parentInode, UFS_DIRECTORY, directory) < 1) {
    cerr << "Error creating directory" << endl;

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
