#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#define SHM_NAME "aravinds_sbaskaran3"

#include "stats.h"

// Mutex variables
pthread_mutex_t mutex;
pthread_mutexattr_t mutexAttribute;

//Global pointer to the shared memory
void* shm_ptr;
stats_t* read_ptr;
int pg_size = 0;
int i = 0;
int maxClients = 0;

void exit_handler(int sig) 
{
    // Unmapping and unlinking the created shared memory
    read_ptr = shm_ptr + 64;
    for(i = 1; i <=  maxClients; i++){
      if(read_ptr->valid == 1){
        kill(read_ptr->pid, SIGTERM);
      }
      read_ptr++;
    }
    munmap(shm_ptr, pg_size);
    shm_unlink(SHM_NAME);       
    exit(0);
}

int main(int argc, char *argv[]) 
{
    //stats_t* read_ptr;
    int iteration = 0, i = 0;

    //Add signal handler
    struct sigaction act;
    act.sa_handler = exit_handler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
	
    // Creating a new shared memory segment
    int fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0660);
    if(fd_shm == -1) {
      exit(1);
    }
    pg_size = getpagesize();
    maxClients = (pg_size / 64) - 1;
    ftruncate(fd_shm, pg_size);
    shm_ptr = mmap(NULL, pg_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);

    // Initializing mutex
    pthread_mutexattr_init(&mutexAttribute);
    pthread_mutexattr_setpshared(&mutexAttribute, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mutex, &mutexAttribute);
    memcpy(shm_ptr, &mutex, sizeof(pthread_mutex_t));

    while (1) {
	// Display stats of active clients
        iteration++;
        read_ptr = shm_ptr + 64;
        for(i = 1; i <= maxClients; i++) {
           if(read_ptr -> valid == 1) {
       	        printf("%d, pid : %d, birth : %s, elapsed : %d s  %0.4lf ms, %s\n", iteration, read_ptr->pid, read_ptr->birth, read_ptr->elapsed_sec, read_ptr->elapsed_msec,read_ptr->clientString);
           }
           read_ptr++;
        }

        sleep(1);
    }

    return 0;
}
