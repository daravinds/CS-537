/* shouldn't trap at stack bound */
#include "types.h"
#include "user.h"

#undef NULL
#define NULL ((void*)0)

#define PGSIZE (4096)

int ppid;
int cnt, d;
#define assert(x) if (x) {} else { \
   printf(1, "%s: %d ", __FILE__, __LINE__); \
   printf(1, "assert failed (%s)\n", # x); \
   printf(1, "TEST FAILED\n"); \
   kill(ppid); \
   exit(); \
}

void worker(void *arg_ptr);
int f(int);

int
main(int argc, char *argv[])
{
   cnt = 0;
   d = 127;
   assert(thread_create(worker, (void *)&d) > 0);
   assert(thread_join() > 0);
   assert(cnt == 126);
   cnt = 0;
   d = 128;
   assert(thread_create(worker, (void *)&d) > 0);
   assert(thread_join() > 0);
   assert(cnt == 127);
   printf(1, "TEST PASSED\n");
   exit();
}

int f(int i) {
  if (i <= 1) return 1;
  cnt++;
  return f(i-1) + i;
}

void
worker(void *arg_ptr) {
    f(*(int *)arg_ptr);
}
