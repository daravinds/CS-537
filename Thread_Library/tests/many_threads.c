/* threads should clean up stacks before exit  */
#include "types.h"
#include "user.h"

#undef NULL
#define NULL ((void*)0)

#define PGSIZE (4096)

int ppid;

#define assert(x) if (x) {} else { \
   printf(1, "%s: %d ", __FILE__, __LINE__); \
   printf(1, "assert failed (%s)\n", # x); \
   printf(1, "TEST FAILED\n"); \
   kill(ppid); \
   exit(); \
}

void worker(void *arg_ptr);

int
main(int argc, char *argv[])
{
   ppid = getpid();
   int i,j;
   for(i = 0; i < 20; i++) {
      for (j = 0; j < 8; j++) 
        assert(thread_create(worker, 0) > 0);
      for (j = 0; j < 8; j++) 
        assert(thread_join() > 0);
   }

   printf(1, "TEST PASSED\n");
   exit();
}

void
worker(void *arg_ptr) {
  int i;
  for (i=0; i<100; i++) ;
}

