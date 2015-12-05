// Phillip Stewart
// CPSC 351, Spring 2015

#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"

#define SHARED_MEMORY_CHUNK_SIZE 1000

const char recvFileName[] = "recvfile";
FILE* fp;

/* Shared memory */
int shmid = 0;
void* smp = 0;

/* Sender pid */
int sender_pid = 0;

/* 'count' tracks the bytes in shared mem */
volatile sig_atomic_t count;

/* 'go' acts as a sort of mutex:
 * It is initially set to 0 and used for busy-waiting,
 * when signaled, the value is set to 1,
 * and then reset to 0 as soon as the busy-wait exits. */
volatile sig_atomic_t go = 0;


/* Sets pid in shared mem and waits to get pid from sender. */
void pidswap()
{
	int pid = getpid();
    *(int*)smp = pid;
	pause();
	go = 0;
    sender_pid = *(int*)smp;
}

/* Sets up the shared memory segment and message queue,
 * calls pidswap() */
void init()
{
	/* Use ftok in order to generate a key. */
	int key;
	if ((key = ftok("keyfile.txt", 'a')) == -1) { 
        perror(" line 52, recv ftok error");
        exit(1);
    }

	/* Get the id of the shared memory segment of size SHARED_MEMORY_CHUNK_SIZE */
	if ((shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0666 | IPC_CREAT)) == -1) {
        perror(" line 58, recv shmget error");
        exit(1);
    }
	
	/* Attach to the shared memory */
    if ((int)(smp = shmat(shmid, NULL, 0)) == -1) {
        perror(" line 64, recv shmat error");
        exit(1);
    }
    
    pidswap();
	
//	printf("key : %d smp: %p\n", key, smp);
//	printf("my pid: %d\n", getpid());
//	printf("sender pid: %d\n", sender_pid);
}


/* Signals sender that it's ready for the count.
 * receives signals while spinning, and then resets spinlock.
 */
void get_count()
{
	count = 0;
	kill(sender_pid, SIGUSR1);
	/* A cheap self locking spinlock...
	 * (reseting the lock is not atomic, but it's good enough)
	 */
 	while (!go);
	go = 0;
}


/* The main loop for receiving data */
void receive()
{
	/* Open the file for writing */
	fp = fopen(recvFileName, "w");
	/* Error checks */
	if(!fp)
	{
		perror(" line 96, recv fopen error");	
		exit(-1);
	}
	
	/* Loop and receive until the sender sets the size to 0, indicating that
	 * there is no more data to send */
	int bytes_written;
	do {
		get_count();
    	if (count == 0) break;
    	
		/* Save the shared memory to file */
		bytes_written = fwrite(smp, sizeof(char), count, fp);
		if(bytes_written != count) {
			perror(" line 110, recv fwrite error");
			exit(1);
		}

	} while (1);
	
	/* Close the file */
	fclose(fp);
}



/* Perfoms the cleanup functions */
void cleanUp()
{
	/* Detach from shared memory */
	if (smp) {
		if (shmdt(smp) == -1) {
    	    perror(" line 128, recv shmdt error");
    	    exit(1);
    	}
    }
    
    if (shmid) {
	    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
    	    perror(" line 135, recv shmctl error");
        	exit(1);
    	}
    }
    
    if (fp)
    	fclose(fp);
}


/* Handles the exit signal */
void ctrlCSignal(int signal)
{
	cleanUp();
	exit(130); // 130 is ^C exit code
}

/* Signal for incrementing the counter manually */
void sig1()
{
	count++;
}

/* Signal for spinlock */
void sig2()
{
	go = 1;
}

/* Signal for setting count to the full buffer size */
void set_full()
{
	count = SHARED_MEMORY_CHUNK_SIZE;
}


/* Main */
int main(int argc, char** argv)
{
	/* Set signal handler for ^C */
	signal(SIGINT, ctrlCSignal);
	
	/* Set message signals */
	signal(SIGUSR1, sig1);
	signal(SIGUSR2, sig2);
	signal(SIGALRM, set_full);
	
	/* run the program */
	init();
	receive();
	cleanUp();
	
	return 0;
}
