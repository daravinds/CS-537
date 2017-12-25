#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define ROOTINO 1  // root i-number
#define BSIZE 512  // block size
#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Special device
#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)
#define DIRSIZ 14
#define IPB (BSIZE / sizeof(struct dinode))
#define DPB (BSIZE / sizeof(struct dirent))

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

// File system super block
struct superblock {
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
};

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+1];   // Data block addresses
};

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};

void* image_ptr;
int *data_blocks_marked_in_use, *data_blocks_in_use, *inodes_marked_in_use, *inodes_in_use, *parent_inode, *lost_inode, *is_directory;
int** children_inodes;
int repair = 0;

void print_error_and_exit(char* error) {
  if(repair == 0) {
    fprintf(stderr, "%s", error);
    exit(1);
  }
/* 
  free(data_blocks_marked_in_use);
  free(data_blocks_in_use);
  free(inodes_marked_in_use);
  free(inodes_in_use);
*/
}

int check_current_and_parent_entries(int current_inode, struct dinode *dinode) {
  int i, j, current_found = 0, parent_found = 0;
  struct dirent* dirent;
  int k = 0;
  for(i = 0; i < NDIRECT; i++) {
    dirent = (struct dirent*) (image_ptr + dinode->addrs[i] * BSIZE);
    for(j = 0; j < DPB; j++, dirent++) {
      if(dirent->inum > 0)
        if((strcmp(dirent->name, ".") != 0) && (strcmp(dirent->name, "..") != 0)) {
          inodes_in_use[dirent->inum] += 1;
          children_inodes[current_inode][k++] = dirent->inum; 
        }
      if(dirent->inum == current_inode && strcmp(dirent->name, ".") == 0)
        current_found = 1;
      if(strcmp(dirent->name, "..") == 0) {
        parent_inode[current_inode] = dirent->inum;
        parent_found = 1;
      }
    }
  }
  uint* indirect_block_ptr;
  indirect_block_ptr = (uint*) (image_ptr + dinode->addrs[NDIRECT] * BSIZE);
  for(i = 0; i < NINDIRECT; i++, indirect_block_ptr++) {
    dirent = (struct dirent*) (image_ptr + *indirect_block_ptr * BSIZE);
    for(j = 0; j < DPB; j++, dirent++)
      if(dirent->inum > 0)
        if((strcmp(dirent->name, ".") != 0) && (strcmp(dirent->name, "..") != 0)) {
          inodes_in_use[dirent->inum] += 1;
          children_inodes[current_inode][k++] = dirent->inum;
        }
      if(dirent->inum == current_inode && strcmp(dirent->name, ".") == 0)
        current_found = 1;
      if(strcmp(dirent->name, "..") == 0) {
        parent_inode[current_inode] = dirent->inum;
        parent_found = 1;
      }
  }
  inodes_in_use[1] = 1;
  children_inodes[current_inode][k] = 0;
  return current_found && parent_found;
}

int main(int argc, char *argv[]) {
  int return_code, i, j, k, root_found = 0, parent_found = 0, l_f = 0;
  if(argc < 2)
    print_error_and_exit("Usage: xv6_fsck <file_system_image>\n");
  int c;
  while((c = getopt(argc, argv, "r::")) != -1) {
    repair = repair || (c == 'r');
  }
  int image_file;
  char* name = argv[argc - 1];
  if(repair == 1) {
    image_file = open(name, O_RDWR);
  }
  else {
    image_file = open(name, O_RDONLY);
  }

  if(image_file < 0)
    print_error_and_exit("image not found.\n");

  struct stat image_file_status;
  return_code = fstat(image_file, &image_file_status);
  assert(return_code == 0);  
  if(repair == 0) {
    image_ptr = mmap(NULL, image_file_status.st_size, PROT_READ, MAP_PRIVATE, image_file, 0);
  }
  else {
    image_ptr = mmap(NULL, image_file_status.st_size, PROT_WRITE, MAP_SHARED, image_file, 0);
  }
  assert(image_ptr != MAP_FAILED);

  struct superblock* sb = (struct superblock*) (image_ptr + BSIZE);
  data_blocks_marked_in_use = (int *) malloc(sb->nblocks * sizeof(int));
  assert(data_blocks_marked_in_use != NULL);
  data_blocks_in_use = (int *) malloc(sb->nblocks * sizeof(int));
  assert(data_blocks_in_use != NULL);
  inodes_marked_in_use = (int *) malloc(sb->ninodes * sizeof(int));
  assert(inodes_marked_in_use != NULL);
  inodes_in_use = (int *) malloc(sb->ninodes * sizeof(int));
  assert(inodes_in_use != NULL);
  parent_inode = (int *) malloc(sb->ninodes * sizeof(int));
  assert(parent_inode != NULL);
  children_inodes = (int**) malloc(sb->ninodes * sizeof(int*));
  assert(children_inodes != NULL);
  lost_inode = (int *) malloc(sb->ninodes * sizeof(int));
  assert(lost_inode != NULL);
  is_directory = (int *) malloc(sb->ninodes * sizeof(int));
  assert(is_directory != NULL);

  for(i = 0; i < sb->ninodes; i++) {
    children_inodes[i] = (int *) malloc(sb->ninodes * sizeof(int));
    assert(children_inodes[i] != NULL);
  }

  struct dinode* inode_table_ptr = (struct dinode*) (image_ptr + 2*BSIZE);
  struct dirent* dirent;
  for(i = 0; i < sb->nblocks; i++) {
    data_blocks_marked_in_use[i] = 0;
    data_blocks_in_use[i] = 0;
  }
  for(i = 0; i < sb->ninodes; i++) {
    inodes_marked_in_use[i] = 0;
    inodes_in_use[i] = 0;
  }
  for(i = 0; i < sb->ninodes; i++) {
    short inode_type = inode_table_ptr->type;
    is_directory[i] = (inode_type == T_DIR);
    if((inode_type != 0) && (inode_type != 1) && (inode_type != 2) && (inode_type != 3)) {
      print_error_and_exit("ERROR: bad inode.\n");
    } else {
      if(inode_type != 0)
        inodes_marked_in_use[i] += 1;
      if(inode_type == T_DIR)
        if(check_current_and_parent_entries(i, inode_table_ptr) == 0)
          print_error_and_exit("ERROR: directory not properly formatted.\n");

      if(i == 1) {
        root_found = 0;
        parent_found = 0;
        if(inode_type != T_DIR)
          print_error_and_exit("ERROR: root directory does not exist.\n");
        for(k = 0; k < NDIRECT; k++) {
          dirent = (struct dirent*) (image_ptr + inode_table_ptr->addrs[k] * BSIZE);
          for(j = 0; j < DPB; j++, dirent++) {
            if(strcmp(dirent->name, "lost_found") == 0)
              l_f = dirent->inum;
            if(dirent->inum == 1) {
              root_found = root_found || (strcmp(dirent->name, ".") == 0);
              parent_found = parent_found || (strcmp(dirent->name, "..") == 0);
            }
          }
        }
        if(root_found == 0 || parent_found == 0)
          print_error_and_exit("ERROR: root directory does not exist.\n");
      }
      uint data_block_accessed;
      for(j = 0; j < NDIRECT; j++) {
        data_block_accessed = inode_table_ptr->addrs[j];
        if(data_block_accessed * BSIZE > image_file_status.st_size)
          print_error_and_exit("ERROR: bad direct address in inode.\n");
        if(data_block_accessed > 0)
          data_blocks_in_use[data_block_accessed] += 1;
      }
      data_blocks_in_use[inode_table_ptr->addrs[NDIRECT]] += 1;
      uint* indirect_block_ptr = (uint*) (image_ptr + inode_table_ptr->addrs[NDIRECT] * BSIZE);
      for(int k = 0; k < NINDIRECT; k++) {
        data_block_accessed = (uint) (*indirect_block_ptr);
        if((data_block_accessed * BSIZE) >= image_file_status.st_size)
          print_error_and_exit("ERROR: bad indirect address in inode.\n");
        if(data_block_accessed > 0)
          data_blocks_in_use[data_block_accessed] += 1;
        indirect_block_ptr++;
      }
    }
    inode_table_ptr++;
  }
  inode_table_ptr = (struct dinode*) (image_ptr + 2 * BSIZE);
  for(i = 0 ; i < sb->ninodes; i++, inode_table_ptr++) {
    if(inode_table_ptr->type == T_FILE)
      if(inode_table_ptr->nlink != inodes_in_use[i])
        print_error_and_exit("ERROR: bad reference count for file.\n");
    if(inode_table_ptr->type == T_DIR)
      if(inodes_in_use[i] > 1)
        print_error_and_exit("ERROR: directory appears more than once in file system.\n");
    // printf("%d: %d, inodes_marked_in_use: %d, inodes_in_use: %d\n", i, inode_table_ptr->type, inodes_marked_in_use[i], inodes_in_use[i]);
    if(inodes_marked_in_use[i] > 0 && inodes_in_use[i] == 0) {
      lost_inode[i] = 1;
      int p;
      for(p = 0; p < NDIRECT && repair; p++) {
        
      }
      print_error_and_exit("ERROR: inode marked use but not found in a directory.\n");
    }
    if(inodes_marked_in_use[i] == 0 && inodes_in_use[i] > 0)
      print_error_and_exit("ERROR: inode referred to in directory but marked free.\n");
  }
  char* bitmap_ptr = image_ptr + (sb->ninodes / IPB + 3) * BSIZE;
  uint bitmap_bytes = sb->nblocks / 8 + 1;
    
  for(i = 0, k = 0; i < bitmap_bytes; i++, bitmap_ptr++)
    for(j = 0; j < 8; j++, k++)
      data_blocks_marked_in_use[k] = (*bitmap_ptr >> (j)) & 1;

  // Setting the initial bits in the bitmap to 1 (Metadata blocks).
  for(k = 0; k < sb->ninodes / IPB + 3 + bitmap_bytes / BSIZE + 1; k++)
    data_blocks_in_use[k] = 1;
  

  for(i = 0 ; i < sb->nblocks; i++) {
    // printf("%d: data_blocks_marked_in_use: %d, data_blocks_in_use: %d\n", i, data_blocks_marked_in_use[i], data_blocks_in_use[i]);
    if(data_blocks_marked_in_use[i] > 0 && data_blocks_in_use[i] == 0)
      print_error_and_exit("ERROR: bitmap marks block in use but it is not in use.\n");
    if(data_blocks_marked_in_use[i] == 0 && data_blocks_in_use[i] > 0)
      print_error_and_exit("ERROR: address used by inode but marked free in bitmap.\n");
    if(data_blocks_in_use[i] > 1)
      print_error_and_exit("ERROR: direct address used more than once.\n");
  }
  inode_table_ptr = (struct dinode*) (image_ptr + 2 * BSIZE);
  int parent_node;
  for(i = 0; i < sb->ninodes; i++, inode_table_ptr++) {  
/*    
    printf("Inode (%d): %d -----> Parent: %d\n", inode_table_ptr->type, i, parent_inode[i]);
    for(j = 0; j < sb->ninodes && children_inodes[i][j]; j++)
      printf("%d ", children_inodes[i][j]);
    printf("\n");
*/
    k = 0;
    parent_found = 0;
    if((inode_table_ptr->type == T_DIR) && (i > 1)) {
      while(children_inodes[parent_inode[i]][k] != 0) {
        if(i == children_inodes[parent_inode[i]][k]) {
          parent_found = 1;
          break;
        }
        k++;
      }
      if(parent_found == 0)
        print_error_and_exit("ERROR: parent directory mismatch.\n");
  
      parent_node = parent_inode[i];
      for(j = 0; j < sb->ninodes; j++)
        parent_node = parent_inode[parent_node];
      if(parent_node != 1)
        print_error_and_exit("ERROR: inaccessible directory exists.\n");
    }
  }
  if(repair) {
    inode_table_ptr = (struct dinode*) (image_ptr + 2 * BSIZE);
    inode_table_ptr += l_f;
    struct dirent *ptr_to_write= (struct dirent *) (image_ptr + inode_table_ptr->addrs[0] * BSIZE);
    ptr_to_write += 2;
    inode_table_ptr = (struct dinode*) (image_ptr + 2 * BSIZE);
    for(j = 0; j < sb->ninodes; j++) {
      if(lost_inode[j]) {
        ptr_to_write->inum = j;
        sprintf(ptr_to_write->name, "repair_%d", j);
        ptr_to_write++;
        if(is_directory[j]) {
	  struct dinode* d = (struct dinode*) (image_ptr + 2 * BSIZE);
          d += j;
          struct dirent* d2 = (struct dirent*) (image_ptr+ d->addrs[0] * BSIZE);
          int p;
          
          for(p = 0; p < DPB; p++, d2++) {
            if(strcmp(d2->name, "..") == 0) {
	      d2->inum = l_f;
		break;
            }
          }
        }
      }
    } 
  }
  exit(0);
}
