#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
  FILE *read_fp, *write_fp;
  int numLines = 0;
  int option;
  int invalid_option = 0;
  char *input, *output;
  input = output = '\0';
  // if(argc != 5)
  // {
  //   fprintf(stderr, "Usage: shuffle -i inputfile -o outputfile\n");
  //   exit(1);
  // }

  opterr = 0; // Silencing error messages from getopt for unallowed flags
  while((option = getopt(argc, argv, "i:o:")) != -1) {
    switch(option) {
      case 'i':
        input = malloc(strlen(optarg)*sizeof(char));
        strcpy(input, optarg);
        break;
      case 'o':
        output = malloc(strlen(optarg)*sizeof(char));
        strcpy(output, optarg);
        break;
      case '?':
        invalid_option = 1; //Set when an invalid flag is encountered in the command line arguments
        break;
    }
  }
  if(invalid_option == 1 || input == '\0' || output == '\0') {
    fprintf(stderr, "Usage: shuffle -i inputfile -o outputfile\n");
    exit(1);
  }
    
  read_fp = fopen(input, "r");
  if(read_fp == NULL)
  {
    fprintf(stderr, "Error: Cannot open file %s\n", input);
    exit(1);
  }
  write_fp = fopen(output, "w");
  if(write_fp == NULL)
  {
    fprintf(stderr, "Error: Cannot open file %s\n", output);
    exit(1);
  }
  struct stat fileStat;
  fstat(fileno(read_fp), &fileStat);
  char *ptr;
  ptr = malloc(fileStat.st_size); // Allocate as many bytes as the size of the file
  if(ptr == NULL) {
    fprintf(stderr, "No memory allocated");
    exit(1);
  }
  int sourceBytes = fread(ptr, 1, fileStat.st_size, read_fp);
  if(sourceBytes < fileStat.st_size)
  {
    exit(1);
  }
  char *ptrCopy = ptr;
  char *ptrCopy2 = ptr;
  while(*ptrCopy2 != '\0')
  {
    if(*ptrCopy2 == '\n')
      numLines++;
    ptrCopy2++;
  }

  char **array;
  array = malloc(sizeof(char*)*numLines); // Create an array to store the starting address of each line of the file (the address of the character following '\n')
  int index = 1;
  array[0] = ptrCopy;
  while(*ptrCopy != '\0') {
    if(*ptrCopy == '\n')
      array[index++] = ptrCopy + 1;
    ptrCopy++;
  }

  int topIndex = 0;
  int bottomIndex = numLines - 1;
  
  while(topIndex <= bottomIndex) {
    char string[512];
    int strIndex = 0;
 
    while(array[topIndex][strIndex] != '\n') {
      string[strIndex] = array[topIndex][strIndex]; // Read one line at a time from the top into 'string'
      strIndex++;
    }
    string[strIndex] = '\n';
    fwrite(string, sizeof(char), strIndex + 1, write_fp); // Write 'string' to the output file
    topIndex++;
    if(topIndex > bottomIndex) // If the pointer from top and bottom have crossed each other, stop processing.
      break;
    strIndex = 0;
    while(array[bottomIndex][strIndex] != '\n') {
      string[strIndex] = array[bottomIndex][strIndex]; // Read one line at a time from the botton into 'string'
      strIndex++;
    }
    string[strIndex] = '\n';
    fwrite(string, sizeof(char), strIndex + 1, write_fp); // Write 'string' into the output file
    bottomIndex--;
  }
  /*free(array);
  free(ptr);
  fclose(read_fp);
  fclose(write_fp);*/
  exit(0);
}
