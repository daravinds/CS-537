#include "types.h"
#include "user.h"
#include "x86.h"
int thread_create(void(*fcn)(void*), void *arg)
{
  void* stack = malloc(4096);
  if(stack == NULL) {
    printf(1, "stack alloc failed\n");
    return -1;
  }
  else {
    printf(1, "alloc ");
    return clone(fcn, arg, stack);
  }
}
int thread_join(void)
{
  int pid;
  void* stack = malloc(sizeof(void*));
  pid = join(&stack);
  //printf(1, "PID in thread_join: %d\n", pid);
  if(pid != -1) {
    printf(1, "free ");
    free(stack);
  }
  return pid;  
}

void lock_init(lock_t *lock)
{
  lock->flag = 0;
}
void lock_acquire(lock_t *lock)
{
  while(xchg(&lock->flag, 1) == 1);
}
void lock_release(lock_t *lock)
{
  lock->flag = 0;
}
