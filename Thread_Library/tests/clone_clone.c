/* threads should be able to clone as well */
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

   assert(thread_create(worker, 0) > 0);
   assert(thread_join() > 0);
   assert(global == 100);

   printf(1, "TEST PASSED\n");
   exit();

}

void
worker(void *arg_ptr) {
    assert(thread_create(worker2, arg_ptr) > 0);
    assert(thread_join() > 0);
    assert(global == 100);
}

void
worker2(void *arg_ptr) {
    int i;
    for (i = 0; i < 100; i++)
	global++;
}
