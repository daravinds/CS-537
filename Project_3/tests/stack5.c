/* stack and heap grow together, reserving 5 pages in between */
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
  *(char*)(157*4096) = 'a';

  // heap run into stack
  uint sz = (uint) sbrk(0);
  uint guardpage = 157*4096 - 5 * 4096;
  assert((int) sbrk(guardpage - sz + 1) == -1);
  assert((int) sbrk(guardpage - sz) != -1);
  assert((int) sbrk(1) == -1);

  // stack run into heap
  int pid = fork();
  if(pid == 0) {
    char* STACK = (char*)(159*4096);

    while (STACK >= (char*)guardpage+5*4096) {
      *STACK = 'a';
      STACK -= 4096;
    }
    // should fail
    *STACK = 'a';
    printf(1, "TEST FAILED\n");
    kill(ppid);
    exit();
  } else {
    wait();
  }

  printf(1, "TEST PASSED\n");
  exit();
}
