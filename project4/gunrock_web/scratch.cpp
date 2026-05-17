#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

#include "StringUtils.h"
#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

int main() {
  Disk *disk = new Disk("./tests/disk_images/a.img", UFS_BLOCK_SIZE);
  LocalFileSystem *fs = new LocalFileSystem(disk);

  // cout << UFS_BLOCK_SIZE << endl; // 4KB
  // cout << disk->numberOfBlocks() << endl;
  // cout << sizeof(int) << endl;

  delete fs;
  delete disk;
  return 0;
}