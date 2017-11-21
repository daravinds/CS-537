#include "types.h"
#include "user.h"
#include "x86.h"
int thread_create(void(*fcn)(void*), void *arg)
{
  void* stack = malloc(4096);
  if(stack == NULL)
    return -1;
  else {
    return clone(fcn, arg, stack);
  }
}
int thread_join(void)
{
  int pid;
  void* stack;
  pid = join(&stack);
  if(pid != -1)
    free(stack);
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
  xchg(&lock->flag, 0);
}
