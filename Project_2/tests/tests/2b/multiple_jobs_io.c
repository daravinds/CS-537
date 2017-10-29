#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"
#define check(exp, msg) if(exp) {} else {				\
  printf(1, "%s:%d check (" #exp ") failed: %s\n", __FILE__, __LINE__, msg); \
  exit();}

int pow2[] = {80000000, 32, 16, 8};
char buf[10000]; // ~10KB

int 
workload(int n) {
  int i, j = 0;
  for(i = 0; i < n; i++)
    j += i * j + 1;
  return j;
}

int
main(int argc, char *argv[])
{
  struct pstat st;
  sleep(10);

  int i, j;
  for (i = 0; i < sizeof(buf); i++)
    buf[i] = 'a';

  char* test_file = "README";
  int fd = open(test_file, 0x002);

  for (i = 0; i <= 60; i++) {
    if (fork() == 0) {
      read(fd, buf, i * 100);

      if (i == NPROC - 4) {
        check(getpinfo(&st) == 0, "getpinfo");
        for(i = 0; i < NPROC; i++) {
          if (st.inuse[i]) {
            printf(1, "pid: %d priority: %d\n", st.pid[i], st.priority[i]);
            if (st.pid[i] > 3) {
                check(st.ticks[i][3] > 0, "Every process at the highest level should use at least 1 timer tick");
            } 
            for (j = 3; j > st.priority[i]; j--) {
              check(st.ticks[i][j] == pow2[j], "incorrect #ticks at this level");
            }
            check(st.ticks[i][j] < pow2[j],
                "#ticks at this level should not exceed the maximum allowed");
          }
        }
        printf(1, "TEST PASSED");
      }
    } else {
      wait();
      break;
    }
  }

  close(fd);
  exit();
}
