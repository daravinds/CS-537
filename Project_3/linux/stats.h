#include <stdbool.h>

#ifndef _STATS_H_
#define _STATS_H_

typedef struct {
  int pid;
  char birth[25];
  char clientString[10];
  int elapsed_sec;
  double elapsed_msec;
  int valid;
} stats_t;
#endif
