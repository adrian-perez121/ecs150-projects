#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"
#include <vector>

using namespace std;


int main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << argv[0] << ": diskImageFile inodeNumber" << endl;
    return 1;
  }

  // Parse command line arguments
  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  int inodeNumber = stoi(argv[2]);

  inode_t inode;
  if (fileSystem->stat(inodeNumber, &inode) != 0) {
    delete fileSystem;
    delete disk;
    cerr << "Error reading file" << endl;
    return 1;
  }

  if (inode.type == UFS_DIRECTORY) {
    delete fileSystem;
    delete disk;
    cerr << "Error reading file" << endl;
    return 1;
  }

  int num_blocks = (inode.size + (UFS_BLOCK_SIZE - 1)) / UFS_BLOCK_SIZE;
  vector<unsigned char> data(inode.size);

  if (fileSystem->read(inodeNumber, data.data(), inode.size) != 0) {
    delete fileSystem;
    delete disk;
    cerr << "Error reading file" << endl;
    return 1;
  }

  cout << "File blocks" << endl;
  for (int i = 0; i < num_blocks; i++) {
    cout << inode.direct[i] << endl;
  }

  cout << endl;

  cout << "File data" << endl;

  for (auto byte : data) {
    cout << byte;
  }

  delete fileSystem;
  delete disk;
  return 0;
}
