#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <string.h>

#define ROOTINO 1  // root i-number
#define BSIZE 512  // block size
#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Special device

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

// File system super block
struct superblock {
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
};

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+1];   // Data block addresses
};

#define DIRSIZ 14

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};

int print_error_and_exit(char* error) {
  fprintf(stderr, "%s", error);
  exit(1);
}

void* image_ptr;
int check_current_and_parent_entries(int current_inode, struct dinode *dinode) {
  int i, j, current_found = 0, parent_found = 0;
  struct dirent* dirent;
  for(i = 0; i < NDIRECT; i++) {
    dirent = (struct dirent*) (image_ptr + dinode->addrs[i] * BSIZE);
    for(j = 0; j < 32; j++) {
      if(dirent->inum == current_inode && strcmp(dirent->name, ".") == 0)
        current_found = 1;
      if(strcmp(dirent->name, "..") == 0) parent_found = 1;
      dirent++;
    }
  }
  return current_found && parent_found;
}

int main(int argc, char *argv[]) {
  int return_code, i, j, k, root_found = 0, parent_found = 0;
  if(argc < 2)
    print_error_and_exit("Usage: xv6_fsck <file_system_image>\n");
  int image_file = open(argv[1], O_RDONLY);  
  if(image_file < 0)
    print_error_and_exit("image not found.\n");

  struct stat image_file_status;
  return_code = fstat(image_file, &image_file_status);
  assert(return_code == 0);
  
  image_ptr = mmap(NULL, image_file_status.st_size, PROT_READ, MAP_PRIVATE, image_file, 0);
  assert(image_ptr != MAP_FAILED);
  struct superblock* sb = (struct superblock*) (image_ptr + BSIZE);
  struct dinode* inode_table_ptr = (struct dinode*) (image_ptr + 2*BSIZE);
  struct dirent* dirent;

  for(i = 0; i < sb->ninodes; i++) {
    short inode_type = inode_table_ptr->type;
    if((inode_type != 0) && (inode_type != 1) && (inode_type != 2) && (inode_type != 3)) {
      print_error_and_exit("ERROR: bad inode.\n");
    } else {
      if(inode_type == T_DIR) {
        if(check_current_and_parent_entries(i, inode_table_ptr) == 0)
          print_error_and_exit("ERROR: directory not properly formatted.\n");
      }

      if(i == 1) {
        root_found = 0;
        parent_found = 0;
        if(inode_type != T_DIR)
          print_error_and_exit("ERROR: root directory does not exist.\n");
        for(k = 0; k < 12; k++) {
          dirent = (struct dirent*) (image_ptr + inode_table_ptr->addrs[k] * BSIZE);
          for(j = 0; j < 32; j++) {
            if(dirent->inum == 1) {
              root_found = root_found || (strcmp(dirent->name, ".") == 0);
              parent_found = parent_found || (strcmp(dirent->name, "..") == 0);
            }
            dirent++;          
          }
        }
        if(root_found == 0 || parent_found == 0)
          print_error_and_exit("ERROR: root directory does not exist.\n");
      }
      for(j = 0; j < NDIRECT; j++) {
        if(inode_table_ptr->addrs[j]*BSIZE > image_file_status.st_size)
          print_error_and_exit("ERROR: bad direct address in inode.\n");
      }
      uint* indirect_block_ptr = (uint*) (image_ptr + inode_table_ptr->addrs[NDIRECT] * BSIZE);
      for(int k = 0; k < 128; k++) {
        if(((uint) (*indirect_block_ptr) * BSIZE) >= image_file_status.st_size)
          print_error_and_exit("ERROR: bad indirect address in inode.\n");
        indirect_block_ptr++;
      }
    } 
    inode_table_ptr++;
  } 
  exit(0);
}
