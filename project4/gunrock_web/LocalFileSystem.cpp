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
  void *buffer[UFS_BLOCK_SIZE];
  this->disk->readBlock(0, buffer);

  // Reading the data into the super pointer
  memcpy(super, buffer, sizeof(super_t));
}

void LocalFileSystem::readInodeBitmap(super_t *super, unsigned char *inodeBitmap) {

}

void LocalFileSystem::writeInodeBitmap(super_t *super, unsigned char *inodeBitmap) {

}

void LocalFileSystem::readDataBitmap(super_t *super, unsigned char *dataBitmap) {

}

void LocalFileSystem::writeDataBitmap(super_t *super, unsigned char *dataBitmap) {

}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {

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

