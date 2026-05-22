#include <iostream>
#include <string>
#include <vector>
#include <assert.h>
#include <string.h>

#include "LocalFileSystem.h"
#include "ufs.h"

using namespace std;


LocalFileSystem::LocalFileSystem(Disk *disk) {
  this->disk = disk;
}

void LocalFileSystem::readSuperBlock(super_t *super) {
  unsigned char buffer[UFS_BLOCK_SIZE];
  this->disk->readBlock(0, (void *) buffer);

  // Reading the data into the super pointer
  memcpy(super, buffer, sizeof(super_t));
}

void LocalFileSystem::readInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
  int bitmap_addr = super->inode_bitmap_addr;
  int bitmap_len = super->inode_bitmap_len;
  int bytes_remaining = (super->num_inodes + 7) / 8; // The number of bytes we have to read to populate the bitmap

  unsigned char buffer[UFS_BLOCK_SIZE];

  for (int i = 0; i < bitmap_len; i++) {
    this->disk->readBlock(bitmap_addr + i, (void *) buffer);
    memcpy(inodeBitmap + (UFS_BLOCK_SIZE) * i, buffer, min(bytes_remaining, UFS_BLOCK_SIZE));
    // We shouldn't ever get to the negatives because bit map length shouldn't have the loop run 
    // make the loop run more than it has to
    bytes_remaining = bytes_remaining - UFS_BLOCK_SIZE;
  }
}

void LocalFileSystem::writeInodeBitmap(super_t *super, unsigned char *inodeBitmap) {

}

void LocalFileSystem::readDataBitmap(super_t *super, unsigned char *dataBitmap) {
  int datamap_addr = super->data_bitmap_addr;
  int datamap_len = super->data_bitmap_len;
  int bytes_remaining = (super->num_data + 7) / 8; // The number of bytes we have to read to populate the bitmap

  unsigned char buffer[UFS_BLOCK_SIZE];

  for (int i = 0; i < datamap_len; i++) {
    this->disk->readBlock(datamap_addr + i, (void *) buffer);
    memcpy(dataBitmap + (UFS_BLOCK_SIZE) * i, buffer, min(bytes_remaining, UFS_BLOCK_SIZE));
    // We shouldn't ever get to the negatives because bit map length shouldn't have the loop run 
    // make the loop run more than it has to
    bytes_remaining = bytes_remaining - UFS_BLOCK_SIZE;
  }
}

void LocalFileSystem::writeDataBitmap(super_t *super, unsigned char *dataBitmap) {

}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {
  int inode_addr = super->inode_region_addr;
  int inode_region_len = super->inode_region_len;
  int bytes_remaining = (super->num_inodes * sizeof(inode_t) + 7) / 8; // Divide by 8 so we know how many bytes we need to read

  unsigned char buffer[UFS_BLOCK_SIZE];

  for (int i = 0; i < inode_region_len; i++) {
    disk->readBlock(inode_addr + i, (void *) buffer);
    memcpy(inodes + (UFS_BLOCK_SIZE) * i, buffer, min(bytes_remaining, UFS_BLOCK_SIZE));
    bytes_remaining = bytes_remaining - UFS_BLOCK_SIZE;
  }
}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {

}

int LocalFileSystem::lookup(int parentInodeNumber, string name) {
  // Two things to look out for
  // - make sure the inode is directory
  // - make sure the name is one of the directory entries
  inode_t inode;
  
  // 0 indicates that everything went well
  if (stat(parentInodeNumber, &inode) != 0) {
    return -EINVALIDINODE;
  }

  if (inode.type != UFS_DIRECTORY) {
    return -EINVALIDINODE;
  }

  super_t super;
  readSuperBlock(&super);

  vector<unsigned char> data_bitmap(super.num_data);
  readDataBitmap(&super, data_bitmap.data());

  // Now that we know it's a directory we have to start checking the data
  int bytes_read = 0;
  int i = 0;
  unsigned char buffer[UFS_BLOCK_SIZE];
  dir_ent_t entry;
  
  while (bytes_read < inode.size && i < DIRECT_PTRS) {
    int block_addr = inode.direct[i];

    // Because bitmaps are relative to the start of a region (block_addr - super.data_region_addr)
    // is done to check for valid data blocks
    int is_valid = (*data_bitmap.data() >> (block_addr - super.data_region_addr)) & 1;

    // Make sure the data is valid to read
    if (!is_valid) {
      // This is more like a data error though
      return -EINVALIDINODE;
    }

    // read the block
    disk->readBlock(block_addr, (void *) buffer);
    int pos = 0; // relative location inside of a block
    while(bytes_read < UFS_BLOCK_SIZE && bytes_read < inode.size) {
      memcpy(&entry, buffer + (pos * sizeof(dir_ent_t)), sizeof(dir_ent_t));

      // the correct name was found
      if (string(entry.name) == name) {
        return entry.inum;
      }

      pos++;
      bytes_read = bytes_read + sizeof(dir_ent_t);
    }

    // Move onto the next block
    i++;
  }

  // The correct name wasn't found
  return -EINVALIDINODE;
}

int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
  // Two things to take into account here:
  // -  it should also not be more than the amount of inodes we can fit
  // - The inode should be readable, in other words it should be a 1 in the bitamp
  super_t super;
  readSuperBlock(&super);

  if (inodeNumber >= super.num_inodes) {
    return -EINVALIDINODE;
  }
  
  vector<unsigned char> inode_bitmap(super.num_inodes);
  readInodeBitmap(&super, inode_bitmap.data());

  // Shift over to the bit representing the inode we want to read
  // & to check if its one
  int is_valid = (*inode_bitmap.data() >> inodeNumber) & 1;

  if (!is_valid) {
    return -ENOTALLOCATED;
  }

  vector<inode_t> inodes(super.num_inodes);
  readInodeRegion(&super, inodes.data());

  // Fill in the pointer
  memcpy(inode, &inodes.at(inodeNumber), sizeof(inode_t));

  return 0;
}

int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {
  return 0;
}

int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
  return 0;
}

int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {
  return 0;
}

int LocalFileSystem::unlink(int parentInodeNumber, string name) {
  return 0;
}

