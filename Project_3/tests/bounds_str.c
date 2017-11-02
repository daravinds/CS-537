/* syscall argument checks (string arg) */
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#undef NULL
#define NULL ((void*)0)

#define assert(x) if (x) {} else { \
   printf(1, "%s: %d ", __FILE__, __LINE__); \
   printf(1, "assert failed (%s)\n", # x); \
   printf(1, "TEST FAILED\n"); \
   exit(); \
}

int
main(int argc, char *argv[])
{
  char *str;
  int fd;

  /* grow the heap a bit (move sz around) */
  assert((int)sbrk(4096 * 60) != -1);

  /* at zero */
  str = (char*) 0x0;
  assert(open(str, O_WRONLY|O_CREATE) == -1);

  /* within null page */
  str = (char*) 0x400;
  assert(open(str, O_WRONLY|O_CREATE) == -1);

  /* below code/heap */
  str = (char*) 0x1fff;
  assert(open(str, O_WRONLY|O_CREATE) == -1);

  /* at data */
  str = "foo";
  fd = open(str, O_WRONLY|O_CREATE);
  assert(fd != -1);
  assert(unlink(str) != -1);

  /* at heap top */
  str = (char*) sbrk(0) - 4;
  strcpy(str, "tmp");
  fd = open(str, O_WRONLY|O_CREATE);
  assert(fd != -1);
  assert(unlink(str) != -1);

  /* spanning heap top */
  str[3] = 'a';
  assert(open(str, O_WRONLY|O_CREATE) == -1);

  /* above heap top */
  str += 4;
  assert(open(str, O_WRONLY|O_CREATE) == -1);

  uint STACK = 159*4096;
  uint USERTOP = 160*4096;

  /* below stack */
  str = (char*) STACK-1;
  assert(open(str, O_WRONLY|O_CREATE) == -1);

  /* at stack */
  str = (char*) STACK;
  strcpy(str, "bar");
  fd = open(str, O_WRONLY|O_CREATE);
  assert(fd != -1);
  assert(unlink(str) != -1);

  /* within stack */
  str = (char*) STACK+1024;
  strcpy(str, "tmp");
  fd = open(str, O_WRONLY|O_CREATE);
  assert(fd != -1);
  assert(unlink(str) != -1);

  /* at stack top */
  str = (char*) USERTOP-2;
  assert(str[0] == '\0');
  strcpy(str, "x");
  fd = open(str, O_WRONLY|O_CREATE);
  assert(fd != -1);
  assert(unlink(str) != -1);
  str[0] = '\0';

  /* spanning stack top */
  str = (char*) USERTOP-1;
  str[0] = 'a';
  assert(open(str, O_WRONLY|O_CREATE) == -1);

  /* above stack top */
  str = (char*) USERTOP;
  assert(open(str, O_WRONLY|O_CREATE) == -1);

  printf(1, "TEST PASSED\n");
  exit();
}
