// Phillip Stewart
// CPSC 351, Spring 2015

#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"

#define SHARED_MEMORY_CHUNK_SIZE 1000

FILE* fp;

/* Shared memory */
int shmid = 0;
void* smp = 0;

/* Receiver pid */
int recv_pid = 0;

/* 'go' acts as a sort of mutex:
 * It is initially set to 0 and used for busy-waiting,
 * when signaled, the value is set to 1,
 * and then reset to 0 as soon as the busy-wait exits. */
volatile sig_atomic_t go = 0;


/* Collects receiver pid and gives pid to receiver.
 * Uses signal and a short pause to ensure recv gets the pid.
 */
void pidswap()
{
	recv_pid = *(int*)smp;
	*(int*)smp = getpid();
	go = 0;
	kill(recv_pid, SIGUSR1);
	usleep(300);
}


/* Sets up the shared memory segment and message queue 
 * Also gets recv_pid from shared mem.
 * and similarly sets pid in memory, then signals recv 
 */
void init()
{
	/*  Use ftok in order to generate a key. */
	int key;
	if ((key = ftok("keyfile.txt", 'a')) == -1) {
        perror(" line 52, sender ftok error");
        exit(1);
    }

	/* Get the id of the shared memory segment of size SHARED_MEMORY_CHUNK_SIZE */
	if ((shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0)) == -1) {
        perror(" line 58, sender shmget error");
        exit(1);
    }
	
	/* Attach to the shared memory */
    if ((int)(smp = shmat(shmid, NULL, 0)) == -1) {
        perror(" line 64, sender shmat error");
        exit(1);
    }
    
    pidswap();
	
//	printf("key : %d smp: %p\n", key, smp);
//	printf("my pid: %d\n", getpid());
//	printf("recv pid: %d\n", recv_pid);
}

/* Signals the receiver with the number of bytes in shared mem.
 * Since the most common case is that the buffer is full,
 * SIGALRM has been repurposed for just that.
 * otherwise, SIGUSR1 is sent repeatedly.
 * Then SIGUSR2 is sent to notify that count has been sent.
 */
void send_count(int count)
{
	/* signal receiver with # bytes read */
	if (count == SHARED_MEMORY_CHUNK_SIZE) {
		kill(recv_pid, SIGALRM);
	} else {
		for (int i=0; i<count; i++) {
			kill(recv_pid, SIGUSR1);
			/* sleep just a little to allow time for increment
			 * (doesn't ensure 100% of signals are caught...)
			 */
			usleep(200);
		}
	}
	
	/* sleep just a little to ensure signals don't cross */
	usleep(100);
	kill(recv_pid, SIGUSR2);
}


/* The send function: reads file into shared memory. */
void send(const char* fileName)
{
	/* Open the file for reading */
	fp = fopen(fileName, "r");
	if(!fp)
	{
		perror(" line 109, sender fopen error");
		exit(-1);
	}
	
	/* Read the whole file */
	int br = 0;
	while(!feof(fp))
	{
		/* A cheap self locking spinlock...
		 * (reseting the lock is not atomic, but it's good enough)
		 */
		while(!go);
		go = 0;
		
		/* Read at most SHARED_MEMORY_CHUNK_SIZE into shared memory.
		 * Store actual # of bytes read. */
		if((br = fread(smp, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp)) < 0) {
			perror(" line 126, sender fread error");
			exit(-1);
		}

		send_count(br);
	}

	/* Send signal done counting with 0 counts. */
	send_count(0);
		
	/* Close the file */
	fclose(fp);
}// end of send




/* Performs the cleanup functions */
void cleanUp()
{
	/* Detach from shared memory */
	if (smp) {
		if (shmdt(smp) == -1) {
	        perror(" line 149, sender shmdt error");
	        exit(1);
	    }
	}
	
	if (fp) {
		fclose(fp);
	}
	
	/* recv removes shared mem segment */
}

/* Signal handler for spinlock
 * allows the lock to be opened before program flow control reaches the spin
 */
void sig1(int signum)
{
	go = 1;
}


/* Main */
int main(int argc, char** argv)
{
	/* Check the command line arguments */
	if (argc < 2)
	{
		fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
		exit(-1);
	}

	/* set signal handler */
	signal(SIGUSR1, sig1);
	
	/* run the program */
	init();
	send(argv[1]);
	cleanUp();
		
	return 0;
}
