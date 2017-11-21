#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since boot.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_clone(void)
{
  void *fcn;
  void *arg;
  void *stack;
  if(argptr(0, (char **) &fcn, sizeof(*fcn)) < 0)
    return -1;
  if(argptr(1, (char **) &arg, sizeof(*arg)) < 0)
    return -1;
  if(argptr(2, (char **) &stack, sizeof(char*)) < 0)
    return -1;
  return clone(fcn, arg, stack);
}

int
sys_join(void)
{
  void** stack;
  if(argptr(0, (char **) &stack, sizeof(char*)) < 0)
    return -1;
  int pid = join(stack);
  return pid;
}

int
sys_cond_init(void)
{
  cond_t* cond;
  if(argptr(0, (char**) &cond, sizeof(cond_t)) < 0)
    return -1;
  cond_init(cond);
  return 0;
}

int
sys_cond_wait(void)
{
  cond_t* cond;
  lock_t* lock;
  if(argptr(0, (char**) &cond, sizeof(cond_t)) < 0)
    return -1;
  if(argptr(1, (char**) &lock, sizeof(lock_t)) < 0)
    return -1;
  cond_wait(cond, lock);
  return 0;
}

int
sys_cond_signal(void)
{
  cond_t* cond;
  if(argptr(0, (char**) &cond, sizeof(cond_t)) < 0)
    return -1;
  cond_signal(cond);
  return 0;
}
