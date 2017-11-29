#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#include "stats.h"
#define SHM_NAME "aravinds_sbaskaran3"


// Mutex variables
pthread_mutex_t* mutex;
stats_t *write_ptr;
int fd_shm = -1;

void exit_handler(int sig) {

    // critical section begins
    pthread_mutex_lock(mutex);
    
    write_ptr->valid = 0;
       
    pthread_mutex_unlock(mutex);
    // critical section ends

    exit(0);
}

int main(int argc, char *argv[]) {
    struct sigaction act;
    act.sa_handler = exit_handler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    // Open shared memory page
    int i;
    stats_t *scan_ptr;
    struct timeval processStartTime, processCurrentTime;
    int pg_size = getpagesize();
    int maxClients = (pg_size / 64) - 1;
    fd_shm = shm_open(SHM_NAME, O_RDWR, 0660);
    if(fd_shm < 0){
      exit(1);
    }
    void* shm_ptr = mmap(NULL, pg_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);     

    mutex = (pthread_mutex_t *)shm_ptr; 
        
    for(i = 1; i <= maxClients; i++) {
    	write_ptr = (stats_t*)(shm_ptr+(i*64));
        if(write_ptr->valid == 0){
          // critical section begins         
          pthread_mutex_lock(mutex);     
          write_ptr->valid = 1;
          pthread_mutex_unlock(mutex);
          // critical section ends
            break;
         }
    }
    
    // Return if shared memory is full. That is currently 64 clients are running
    if(i > maxClients){
       exit(0);
    }

   write_ptr->pid = getpid();
   if(strlen(argv[1]) > 9) {
     exit(1);
   }
   strncpy(write_ptr->clientString, argv[1], 9);
   gettimeofday(&processStartTime, NULL);   
   snprintf(write_ptr->birth, 25, "%s", ctime(&processStartTime.tv_sec));  
     
    while (1) {        
	// Construct this clients stat object
        gettimeofday(&processCurrentTime, NULL);
        write_ptr->elapsed_sec = processCurrentTime.tv_sec - processStartTime.tv_sec;
        long usec  = processCurrentTime.tv_usec - processStartTime.tv_usec;
        write_ptr->elapsed_msec = usec / 1000.0f;       

        sleep(1);

	// Print active clients
        printf("Active clients :");
        scan_ptr = (stats_t*)shm_ptr;
        scan_ptr++;

	for(i = 1; i <= maxClients; i++){
          if(scan_ptr->valid == 1) {
            printf(" %d", scan_ptr->pid);
          } 
          scan_ptr++;
        }
        printf("\n");
    }
    return 0;
}
