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
  int bitmap_addr = super->inode_bitmap_addr;
  int bitmap_len = super->inode_bitmap_len;
  int bytes_remaining = (super->num_inodes + 7) / 8;

  unsigned char buffer[UFS_BLOCK_SIZE];

  for (int i = 0; i < bitmap_len; i++) {
    memcpy(buffer, inodeBitmap + (UFS_BLOCK_SIZE) * i, min(bytes_remaining, UFS_BLOCK_SIZE));
    this->disk->writeBlock(bitmap_addr + i, buffer);

    bytes_remaining = bytes_remaining - UFS_BLOCK_SIZE;
  }

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
  int bitmap_addr = super->data_bitmap_addr;
  int bitmap_len = super->data_bitmap_len;
  int bytes_remaining = (super->num_data + 7) / 8;

  unsigned char buffer[UFS_BLOCK_SIZE];

  for (int i = 0; i < bitmap_len; i++) {
    memcpy(buffer, dataBitmap + (UFS_BLOCK_SIZE) * i, min(bytes_remaining, UFS_BLOCK_SIZE));
    this->disk->writeBlock(bitmap_addr + i, buffer);

    bytes_remaining = bytes_remaining - UFS_BLOCK_SIZE;
  }

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
  int inode_addr = super->inode_region_addr;
  int inode_region_len = super->inode_region_len;
  int bytes_remaining = super->num_inodes * sizeof(inode_t); 

  unsigned char buffer[UFS_BLOCK_SIZE];

  for (int i = 0; i < inode_region_len; i++) {
    memcpy(buffer, inodes + (UFS_BLOCK_SIZE / sizeof(inode_t)) * i, min(bytes_remaining, UFS_BLOCK_SIZE));
    disk->writeBlock(inode_addr + i, (void *) buffer);
    bytes_remaining = bytes_remaining - UFS_BLOCK_SIZE;
  }
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

  if (read(parentInodeNumber, dir_entries.data(), inode.size) != 0) {
    return -EINVALIDINODE;
  }
  
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

  if (inodeNumber < 0|| inodeNumber >= super.num_inodes) {
    return -EINVALIDINODE;
  }
  
  vector<unsigned char> inode_bitmap(super.num_inodes / 8);
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

  vector<unsigned char> inode_bitmap(super.num_inodes / 8);
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
  if (size < 0) {
    return -EINVALIDSIZE;
  }

  // If size is larger then only read the bytes in the object
  int bytes_remaining = min(size, inode.size);
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
  // Make sure the proper bit maps are change and make sure that the file is added to the directory entry on disk
  super_t super;
  inode_t parent_inode;
  int child_inode_number;
  inode_t child_inode = {type, 0}; // an initial value that can get overwritten later

  readSuperBlock(&super);

  if (stat(parentInodeNumber, &parent_inode) != 0) {
    return -EINVALIDINODE;
  }

  if (parent_inode.type != UFS_DIRECTORY) {
    return -EINVALIDTYPE;
  }

  // .size() doesn't take into account \0 we have to take it into account
  if (name.size() + 1 > DIR_ENT_NAME_SIZE) {
    return -EINVALIDNAME;
  }

  if ((child_inode_number = lookup(parentInodeNumber, name)) < 0) {
    // Find the first available inode
    vector<unsigned char> inode_bitmap((super.num_inodes + 7) / 8);
    readInodeBitmap(&super, inode_bitmap.data());

    int empty_inode;
    int new_inode_num = 0;
    int byte_index = 0;
    int bit_location = 0;
    for (auto byte : inode_bitmap) {

      for (bit_location = 0; bit_location < 8 && new_inode_num < super.num_inodes; bit_location++) { // 8 represents the size of a byte
        empty_inode = ((~byte) >> bit_location) & 1;

        if (empty_inode) {
          // add the inode to the bitmap, so we can write it in later
          byte_index = new_inode_num / 8;
          inode_bitmap.at(byte_index) = (byte | (1 << bit_location));

          break;
        }

        new_inode_num++;
      }

      if (empty_inode) {
        break;
      }
    }

    // - There has be an inode available for the file to take up 
    if (!(new_inode_num < super.num_inodes)) {
        return -ENOTENOUGHSPACE;
    }

    child_inode_number = new_inode_num;

    // This means we have to create the child ourselves
    vector<inode_t> inodes(super.num_inodes);

    readInodeRegion(&super, inodes.data());
    inodes.at(new_inode_num) = child_inode; // This was initialized in the beginning of the function
    writeInodeRegion(&super, inodes.data());

    // Now that you have written in inode make sure to update the bit map
    // the byte_index and bit_location shouldn't have changed from the loop broke
    inode_bitmap.at(byte_index) = (inode_bitmap.at(byte_index) | (1 << bit_location));
    writeInodeBitmap(&super, inode_bitmap.data());

    // If the inode is a directory it should already have two files inside of it: "." and ".."
    if (child_inode.type == UFS_DIRECTORY) {
      vector<dir_ent_t> initial_entries;
      dir_ent_t parent_dir = {"..\0", parentInodeNumber};
      dir_ent_t current_dir = {".\0", child_inode_number};

      initial_entries.push_back(parent_dir);
      initial_entries.push_back(current_dir);

      // To make sure we don't run out of space
      if (write(child_inode_number, initial_entries.data(), initial_entries.size() * sizeof(dir_ent_t), true) != (int) (initial_entries.size() * sizeof(dir_ent_t))) {
        return -ENOTENOUGHSPACE;
      }
    }

    // Make sure to update the directory entries of parent
    vector<dir_ent_t> dir_entries(parent_inode.size / sizeof(dir_ent_t));
    read(parentInodeNumber, dir_entries.data(), parent_inode.size);

    dir_ent_t new_entry;
    vector<char> characters(name.begin(), name.end());
    characters.push_back('\0');

    strcpy(new_entry.name, characters.data());
    new_entry.inum = child_inode_number;

    dir_entries.push_back(new_entry);

    int total_bytes = dir_entries.size() * sizeof(dir_ent_t);
    
    // - There also has to be space in the directory data for the file to be listed there
    // If we couldn't write in everything, the disk doesn't have enough space to store the directory data
    if (write(parentInodeNumber, dir_entries.data(), total_bytes, true) != total_bytes) {
      return -ENOTENOUGHSPACE;
    }


    return child_inode_number;
  } else {
    // The child exists and we have to check if it is of the correct type
    if (stat(child_inode_number, &child_inode) != 0) { // This shouldn't ever happen because we did the lookup but I'm paranoid
      return -EINVALIDINODE;
    } 

    if (child_inode.type != type) {
      return -EINVALIDTYPE;
    }

    return child_inode_number;
  }


  return 0;
}

int LocalFileSystem::write(int inodeNumber, const void *buffer, int size, bool to_dir = false) {
  // to_dir was added because the regular user shouldn't be able to write to a directory but the file system 
  // should be able to write to directories in the case of regular file and directory creations
  inode_t inode;
  super_t super;

  readSuperBlock(&super);

  if (stat(inodeNumber, &inode) != 0) {
    return -EINVALIDINODE;
  }

  if (inode.type == UFS_DIRECTORY && !to_dir) { // you are not supposed to writing to a directory
    return -EINVALIDTYPE;
  }

  // Game plan:
  // Now that you have to write amount of blocks, do your write 
  // Edge case, if you need more blocks than there are available just take up the max amount of blocks you can 
  // keep track of all the bytes being written 

  // - Figure out how many blocks you need
  int old_blocks = (inode.size + (UFS_BLOCK_SIZE - 1)) / UFS_BLOCK_SIZE;
  int new_blocks = min((size + (UFS_BLOCK_SIZE - 1)) / UFS_BLOCK_SIZE, DIRECT_PTRS);

  // The current number of data blocks the current inode now holds
  int updated_blocks = old_blocks;
  vector<unsigned char> data_bitmap((super.num_data + 7) / 8);
  readDataBitmap(&super, data_bitmap.data());

  // BEFORE ANY WRITING: Deallocate or allocate so you get to the amount of blocks you need
  if (old_blocks > new_blocks) {
    // This means you need to deallocate the blocks you don't need
    
    int current_block_ptr = old_blocks - 1;

    // Find out which blocks you won't need using the pointers 
    // Deallocate these blocks by updating the data bitmap
    while (current_block_ptr > new_blocks - 1) {
      int block_addr = inode.direct[current_block_ptr];

      // Note: Bitmap indexes identify disk blocks relative to the start of a region.
      int relative_block_adr = block_addr - super.data_region_addr;
      int byte_index = relative_block_adr / 8;
      int byte_pos = relative_block_adr % 8;

      data_bitmap.at(byte_index) = (data_bitmap.at(byte_index) & (~(1 << byte_pos)));

      current_block_ptr--;
    }

    old_blocks = new_blocks;

  } else if (old_blocks < new_blocks) {
    // This means that you have to allocate new blocks 
    int current_block_ptr = old_blocks; // This where each new block addr will be put
    int block_addr;
    
    for (int i = 0; i < super.num_data && updated_blocks < new_blocks && current_block_ptr < DIRECT_PTRS; i++) {
      int byte = i / 8;
      int byte_pos = i % 8;

      // Similar to the is_valid logic except the answer is negated
      int is_empty = ~(data_bitmap.at(byte) >> byte_pos) & 1;
      
      if (is_empty) {
        // Because bit map locations are relative
        block_addr = i + super.data_region_addr;
        inode.direct[current_block_ptr] = block_addr;
        data_bitmap.at(byte) = (data_bitmap.at(byte) | (1 << byte_pos));

        current_block_ptr++;
        updated_blocks++;
      }
    }
  }
  
  writeDataBitmap(&super, data_bitmap.data());
  
  // Now that you have the proper amount of blocks, write to the data bitmap to
  int bytes_written = 0;
  int block_ptr = 0;
  unsigned char disk_buffer[UFS_BLOCK_SIZE];

  // Stop until you reach the size or you can't write anymore
  while (bytes_written < size && block_ptr < updated_blocks) {
    int bytes_being_written = min(size - bytes_written, UFS_BLOCK_SIZE);
    int block_addr = inode.direct[block_ptr];
    
    // We can also use block ptr as an index for where in the buffer we are
    memcpy(disk_buffer, (unsigned char *) buffer + (block_ptr * UFS_BLOCK_SIZE), bytes_being_written);
    disk->writeBlock(block_addr, disk_buffer);

    block_ptr++;
    bytes_written = bytes_written + bytes_being_written;

  }

  // Now that you know how many bytes you have written make sure to update the inode and write that 
  // to disk as well
  inode.size = bytes_written;
  vector<inode_t> inodes(super.num_inodes);
  readInodeRegion(&super, inodes.data());

  inodes.at(inodeNumber) = inode;
  writeInodeRegion(&super, inodes.data());

  // at last you are done and return the number of nodes written
  return bytes_written;
}

int LocalFileSystem::unlink(int parentInodeNumber, string name) {
  super_t super;
  inode_t parent_inode;
  int child_number;
  inode_t child_inode;

  readSuperBlock(&super);

  // Make sure inode is valid...
  if (stat(parentInodeNumber, &parent_inode) < 0) {
    return -EINVALIDINODE;
  }

  // ...and that it is a directory...
  if (parent_inode.type != UFS_DIRECTORY) {
    return -EINVALIDINODE;
  }
  
  // Invalid name
  if (name.size() + 1 > DIR_ENT_NAME_SIZE) {
    return -EINVALIDNAME;
  }
  
  // You can't unlink '.' or '..'
  if (name == "." || name == "..") {
    return -EUNLINKNOTALLOWED;
  }

  
  // The condition for checking if a chile exists
  if ((child_number = lookup(parentInodeNumber, name)) >= 0) {
    stat(child_number, &child_inode);
    int byte_pos = 0;
    int bit_pos = 0;

    // The size being two implies the only two entries are "." and "..", hence it's empty
    if (child_inode.type == UFS_DIRECTORY && child_inode.size / sizeof(dir_ent_t) != 2) {
      return -EDIRNOTEMPTY;
    }

    // Deallocate the inode
    vector<unsigned char> inode_bitmap((super.num_inodes + 7) / 8);
    readInodeBitmap(&super, inode_bitmap.data());
    byte_pos = child_number / 8;
    bit_pos = child_number % 8;
    // negate to move a 1 to the position (marks the inode as not in use) and negate again to flip the meaning of the bits
    inode_bitmap.at(byte_pos) = ~((~inode_bitmap.at(byte_pos)) | (1 << bit_pos));
    writeInodeBitmap(&super, inode_bitmap.data());

    // Deallocate the data the inode is holding
    int blocks = (child_inode.size + (UFS_BLOCK_SIZE - 1)) / UFS_BLOCK_SIZE;
    vector<unsigned char> data_bitmap((super.num_data + 7) / 8);
    readDataBitmap(&super, data_bitmap.data());

    for (int i = 0; i < blocks; i++) {
      // Again because the bitmap indices are relative
      int block_bitmap_index = child_inode.direct[i] - super.data_region_addr;
      byte_pos = block_bitmap_index / 8;
      bit_pos = block_bitmap_index % 8;
      data_bitmap.at(byte_pos) = ~((~data_bitmap.at(byte_pos)) | (1 << bit_pos));
    } 

    writeDataBitmap(&super, data_bitmap.data());

    // Remove the directory entry
    vector<dir_ent_t> old_parent_entries(parent_inode.size / sizeof(dir_ent_t));
    read(parentInodeNumber, old_parent_entries.data(), parent_inode.size);
    vector<dir_ent_t> new_parent_entries;
    
    for (auto entry : old_parent_entries) {
      if (string(entry.name) == name) {
        continue;
      }

      new_parent_entries.push_back(entry);
    }

    // Your writing less data in so it SHOULD work, right?
    write(parentInodeNumber, new_parent_entries.data(), new_parent_entries.size() * sizeof(dir_ent_t), true);
  }

  return 0;
}

