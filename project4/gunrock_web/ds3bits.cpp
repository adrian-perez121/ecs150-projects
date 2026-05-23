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
  if (argc != 2) {
    cerr << argv[0] << ": diskImageFile" << endl;
    return 1;
  }

  // Parse command line arguments
  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);

  super_t super;
  fileSystem->readSuperBlock(&super);


  cout << "Super" << endl;

  cout << "inode_region_addr "  << super.inode_region_addr << endl;

  cout << "inode_region_len " << super.inode_region_len << endl;

  cout << "num_inodes " << super.num_inodes << endl;

  cout << "data_region_addr " << super.data_region_addr << endl;

  cout << "data_region_len " << super.data_region_len << endl;

  cout << "num_data " << super.num_data << endl;

  cout << endl;

  cout << "Inode bitmap" << endl;
  vector<unsigned char> inode_bitmap((super.num_inodes + 7) / 8);
  fileSystem->readInodeBitmap(&super, inode_bitmap.data());

  for (auto byte : inode_bitmap) {
    cout << (unsigned int) byte << " ";
  }

  cout << endl << endl;

  cout << "Data bitmap" << endl;
  vector<unsigned char> data_bitmap((super.num_data + 7) / 8);
  fileSystem->readDataBitmap(&super, data_bitmap.data());

  for (auto byte : data_bitmap) {
    cout << (unsigned int) byte << " ";
  }

  cout << endl;
  
  delete fileSystem;
  delete disk;
  return 0;
}
