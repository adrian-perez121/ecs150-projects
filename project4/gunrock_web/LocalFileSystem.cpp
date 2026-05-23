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
  int bytes_remaining = super->num_inodes * sizeof(inode_t); 

  unsigned char buffer[UFS_BLOCK_SIZE];

  for (int i = 0; i < inode_region_len; i++) {
    disk->readBlock(inode_addr + i, (void *) buffer);
    memcpy(inodes + (UFS_BLOCK_SIZE / sizeof(inode_t)) * i, buffer, min(bytes_remaining, UFS_BLOCK_SIZE));
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

  vector<dir_ent_t> dir_entries(inode.size / sizeof(dir_ent_t));

  read(parentInodeNumber, dir_entries.data(), inode.size);
  
  // Implement this by doing a read
  // and then just checking each item in the vector and seeing if a name matches
  for (auto entry : dir_entries) {
    if (string(entry.name) == name) {
      return entry.inum;
    }
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
  int byte = inodeNumber / 8;
  int byte_pos = inodeNumber % 8;
  int is_valid = (inode_bitmap.at(byte) >> byte_pos) & 1;

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
  inode_t inode;
  super_t super;

  if (stat(inodeNumber, &inode) != 0) {
    return -EINVALIDINODE;
  }

  readSuperBlock(&super);

  vector<unsigned char> inode_bitmap(super.num_inodes);
  vector<unsigned char> data_bitmap(super.num_data);

  readInodeBitmap(&super, inode_bitmap.data());
  readDataBitmap(&super, data_bitmap.data());

  int byte = inodeNumber / 8;
  int byte_pos = inodeNumber % 8;
  int is_valid = (inode_bitmap.at(byte) >> byte_pos) & 1;

  if (!is_valid) {
    return -EINVALIDINODE;
  }

  // Make sure we can read the amount of bytes we are being asked of 
  if (size < 0 || size > inode.size) {
    return -EINVALIDSIZE;
  }

  int bytes_remaining = size;
  int i = 0;
  int block_addr;
  unsigned char disk_buffer[UFS_BLOCK_SIZE];

  while (bytes_remaining > 0) {
    // Because bitmaps are relative to the start of a region (block_addr - super.data_region_addr)
    // is done to check for valid data blocks
    int bytes_read = min(UFS_BLOCK_SIZE, bytes_remaining);
    block_addr = inode.direct[i];
    // Because the bitmap is relative
    int relative_block_addr = block_addr - super.data_region_addr;
    int byte = relative_block_addr / 8;
    int byte_pos = relative_block_addr % 8;
    is_valid = (data_bitmap.at(byte) >> byte_pos) & 1;

    // Make sure the data is valid to read
    if (!is_valid) {
      // This is more like a data error though
      return -EINVALIDINODE;
    }

    disk->readBlock(block_addr, disk_buffer);

    memcpy((void *) ((unsigned char *) (buffer) + (i * UFS_BLOCK_SIZE)), disk_buffer, bytes_read);

    bytes_remaining = bytes_remaining - bytes_read;
    i++;
  }

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

