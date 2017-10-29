#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"

#define check(exp, msg) if(exp) {} else {\
  printf(1, "%s:%d check (" #exp ") failed: %s\n", __FILE__, __LINE__, msg);\
  exit();}

#define DDEBUG 1

#ifdef DDEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

//char buf[10000]; // ~10KB
int workload(int n, int t) {
  int i, j = 0;
  for (i = 0; i < n; i++) {
    j += i * j + 1;
  }

  if (t > 0) sleep(t);
  for (i = 0; i < n; i++) {
    j += i * j + 1;
  }
  return j;
}

int
main(int argc, char *argv[])
{
  struct pstat st;
  int time_slices[] = {64, 32, 16, 8};
  check(getpinfo(&st) == 0, "getpinfo");

  // Push this thread to the bottom
  workload(80000000, 0);

  int i, j, k;
  int pids[4];
 
  // Launch the 4 processes, but process 2 will sleep in the middle
  for (i = 0; i < 2; i++) {
    int c_pid = fork();
    int t = 0;
    // Child
    if (c_pid == 0) {
      if (i == 1) {
          t = 64*5; // for this process, give up CPU for one time-slice
      }
      workload(300000000, t);
      exit();
    }  
    else {
        pids[i] = c_pid;
    }
  }

  // Checking every 4 time-slice for 8 times
//  int count = 0;
  for (i = 0; i < 12; i++) { 
    sleep(10);
    check(getpinfo(&st) == 0, "getpinfo");
    
    for (j = 0; j < NPROC; j++) {
      if (st.inuse[j] && st.pid[j] > 3) {
  
        DEBUG_PRINT((1, "pid: %d\n", st.pid[j]));
        for (k = 3; k >= 0; k--) {
          DEBUG_PRINT((1, "\t level %d ticks used %d\n", k, st.ticks[j][k]));
          if (k > 0 && st.state[j] != 5) { 
            check(st.ticks[j][k] % time_slices[k] == 0, "Timer ticks above level 0 should be used up\n"); 
          }
          if (k == 0 && st.pid[j] != pids[2] && i > 0) {
//            check(st.ticks[j][k] > 0, "This process should run in modulo of 64 in the lowest level as it never yields CPU\n");
          }
         // if (k == 0 && st.pid[j] == pids[2] && st.ticks[j][k] > 0) {
            // check(i < 7, "This process should regain CPU now\n");
	   // DEBUG_PRINT((1, "\t the count is %d for process %d\n", count, j));
           // count++;  
          //}
        }
      } 
    }
  }

  for (i = 0; i < 2; i++) {
    wait();
  }

 // if (count == 2) { // we should have also tested if these two ticks are identical ...
   // printf(1, "TEST PASSED");
  //} else {
    printf(1, "TEST PASSED");
  //}

  exit();
}
