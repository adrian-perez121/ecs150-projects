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
  this->readSuperBlock(this->super);
}

LocalFileSystem::~LocalFileSystem() {
  delete super;
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
  return 0;
}

int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
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

