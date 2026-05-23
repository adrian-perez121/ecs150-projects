#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <sstream>

#include "StringUtils.h"
#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;


// Use this function with std::sort for directory entries
bool compareByName(const dir_ent_t& a, const dir_ent_t& b) {
    return std::strcmp(a.name, b.name) < 0;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << argv[0] << ": diskImageFile directory" << endl;
    cerr << "For example:" << endl;
    cerr << "    $ " << argv[0] << " tests/disk_images/a.img /a/b" << endl;
    return 1;
  }

  // parse command line arguments

  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  string directory = string(argv[2]);

  stringstream ss(directory);
  string path_section;
  vector<string> path;

  // since the first character is '/' in a path, which means the actual 
  // directories in the path start at index 1 
  while (getline(ss, path_section, '/')) {
    path.push_back(path_section);
  }

  int current_dir = 0;
  size_t part_in_path = 1;
  // Getting to the file we want but stop before the last item because that 
  // is what we are performing ls on
  while (part_in_path < path.size()) {
    current_dir = fileSystem->lookup(current_dir, path.at(part_in_path));

    if (current_dir < 0) {
      cerr << "Directory not found" << endl;
      return 1;
    }
    part_in_path++;
  }

  inode_t inode;
  super_t super;
  fileSystem->readSuperBlock(&super);
  vector<unsigned char> bitmap((super.num_inodes + 7)/8);
  fileSystem->readInodeBitmap(&super, bitmap.data());

  if (fileSystem->stat(current_dir, &inode) != 0) {
    cerr << "Directory not found" << endl;
    return 1;
  } 

  if (inode.type == UFS_REGULAR_FILE) {
    cout << current_dir;
    cout <<  "\t" + path.back() << endl;
  } else if (inode.type == UFS_DIRECTORY) {
    vector<dir_ent_t> entries(inode.size / sizeof(dir_ent_t));
    fileSystem->read(current_dir, entries.data(), inode.size);
    sort(entries.begin(), entries.end(), compareByName);
    
    for (auto entry : entries) {
      cout << entry.inum;
      cout << "\t" + string(entry.name) << endl;
    }

  } else {
    delete disk;
    delete fileSystem;
    cerr << "Directory not found" << endl;
    return 1;
  }

  delete disk;
  delete fileSystem;

  return 0;
}
