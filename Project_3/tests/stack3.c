/* stack should only grow one page at a time */
#include "types.h"
#include "user.h"

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
  int ppid = getpid();

  // ensure they actually placed the stack high...
  *(char*)(159*4096) = 'a';

  // should work fine
  *(char*)(159*4096-1) = 'a';

  int pid = fork();
  if(pid == 0) {
    // should fail
    *(char*)(157*4096-1) = 'a';
    printf(1, "TEST FAILED\n");
    kill(ppid);
    exit();
  } else {
    wait();
  }

  printf(1, "TEST PASSED\n");
  exit();
}
