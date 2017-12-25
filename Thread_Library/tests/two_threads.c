/* process should wait for thread that finishes first */
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
void worker2(void *arg_ptr);

int
main(int argc, char *argv[])
{
   ppid = getpid();
   int thread1, thread2;

   assert((thread1 = thread_create(worker, 0)) > 0);
   assert((thread2 = thread_create(worker2, 0)) > 0);
   assert(thread_join() == thread2);
   assert(global == 2);
   assert(thread_join() == thread1);
   assert(global == 1);

   printf(1, "TEST PASSED\n");
   exit();

}

void
worker(void *arg_ptr) {
    sleep(100);
    global = 1;
}

void
worker2(void *arg_ptr) {
    global = 2;;
}
