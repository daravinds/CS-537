#ifndef _TYPES_H_
#define _TYPES_H_

// Type definitions

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;
#ifndef NULL
#define NULL (0)
#endif

typedef struct __lock_t {
 uint flag;
} lock_t;

typedef struct __cond_t {
  int front, rear, empty;
  int pids[8];
  lock_t *lock;
} cond_t;

#endif //_TYPES_H_
