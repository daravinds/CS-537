#include "cs537.h"
#include "request.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

int buffSize;
int workerThreadCount;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;

int bufffill_ptr = 0;
int buffuse_ptr = 0;
int buffCount = 0;
pthread_t* threadptr;
int* buffer;

// CS537: Parse the new arguments too
void getargs(int *port, int argc, char *argv[])
{
    if (argc != 4) {
	fprintf(stderr, "Usage: %s <port> <threads> <buffer>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    workerThreadCount = atoi(argv[2]);
    if(workerThreadCount < 1) {
        fprintf(stderr, "Usage: %s <port> <threads> <buffer>\n", argv[0]);
        exit(1);
    }
    buffSize = atoi(argv[3]);
    if(buffSize < 1) {
        fprintf(stderr, "Usage: %s <port> <threads> <buffer>\n", argv[0]);
        exit(1);
    }
}

void put(int connfd) 
{
  buffer[bufffill_ptr] = connfd;
  bufffill_ptr = (bufffill_ptr + 1) % buffSize;
  buffCount++;
}

int get() {
   int connfd = buffer[buffuse_ptr];
   buffuse_ptr = (buffuse_ptr + 1) % buffSize;
   buffCount--;
   return connfd;
}


// Consumer Logic
void*  workerThreadStartRoutine(void* arg) {
    int connfd = 0;
    while(1) {
        pthread_mutex_lock(&lock);
        while(buffCount < 1) {
           pthread_cond_wait(&fill, &lock);
        }
        connfd = get();
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&lock);
        requestHandle(connfd);
        Close(connfd);
    }
}

int main(int argc, char *argv[])
{
    int i,listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    getargs(&port, argc, argv);
    
    //Allocate buffer
    buffer = (int*) malloc(sizeof(int)*buffSize);
    // Allocate space for consumer/worker threads 
    threadptr = (pthread_t*) malloc(sizeof(pthread_t)* workerThreadCount);
    for(i = 0; i < workerThreadCount; i++) {
        int err = pthread_create(&(threadptr[i]), NULL, &workerThreadStartRoutine, NULL);
        if (err != 0) { 
            printf("\ncan't create thread :[%s]", strerror(err));
            exit(1);
        }          
    }

    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        
        // Main Thread: Producer thread which fills the buffer
	// 
	// CS537: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	//
 
	//Producer Logic
        pthread_mutex_lock(&lock);
        while (buffCount == buffSize) {        
           pthread_cond_wait(&empty, &lock);
        }
        put(connfd);
        pthread_cond_signal(&fill);
        pthread_mutex_unlock(&lock);
    }
    free(buffer);
    free(threadptr);
}


    


 
