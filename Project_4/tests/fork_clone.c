/* threads across processes should not share data */
#include "types.h"
#include "user.h"

#undef NULL
#define NULL ((void*)0)

#define PGSIZE (4096)
int ppid;
int global = 0;
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
   int p;
   ppid = getpid();
   for (p=0; p<2; p++) {
       if (fork() == 0) {
           assert(global == 0);
           assert(thread_create(worker, 0) > 0);
           assert(thread_join() > 0);
           assert(global == 100);
           exit();
       }
   }
   for (p=0;p<2;p++) wait();
   printf(1, "TEST PASSED\n");
   exit();

}

void
worker(void *arg_ptr) {
    int i;
    for (i=0;i<100;i++)
        global++;
}
