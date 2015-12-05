// Phillip Stewart
// CPSC 351, Spring 2015

#include <sys/shm.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"

#define SHARED_MEMORY_CHUNK_SIZE 1000

/* Shared memory identifier, message queue identifier, shared memory pointer */
int shmid = 0;
int msqid = 0;
void* smp = 0;


/* Sets up the shared memory segment and message queue */
void init()
{
	/*  Use ftok in order to generate a key. */
	int key;
	if ((key = ftok("keyfile.txt", 'a')) == -1) {
        perror(" line 25, sender ftok error");
        exit(1);
    }

	/* Get the id of the shared memory segment of size SHARED_MEMORY_CHUNK_SIZE */
	if ((shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0)) == -1) {
        perror(" line 31, sender shmget error");
        exit(1);
    }
	
	/* Attach to the shared memory */
    if ((int)(smp = shmat(shmid, NULL, 0)) == -1) {
        perror(" line 37, sender shmat error");
        exit(1);
    }
	
	/* Attach to the message queue */
	if ((msqid = msgget(key, 0666)) == -1) {
        perror(" line 43, sender msgget error");
        exit(1);
    }
}


/* Performs the cleanup functions */
void cleanUp()
{
	/* Detach from shared memory */
	if (smp)
	if (shmdt(smp) == -1) {
        perror(" line 55, sender shmdt error");
        exit(1);
    }

	/* Removing message queue and shared memory segment
	 * left for recv to handle. */
}


/* The send function: reads file into shared memory.
 * @param fileName - the name of the file */
void send(const char* fileName)
{
	/* Open the file for reading */
	FILE* fp = fopen(fileName, "r");
	if(!fp)
	{
		perror(" line 72, sender fopen error");
		exit(-1);
	}
	
	/* Message used for send and receive */
	message msg;
	
	/* Read the whole file */
	while(!feof(fp))
	{
		/* Read at most SHARED_MEMORY_CHUNK_SIZE into shared memory.
		 * Store actual # of bytes read. */
		if((msg.size = fread(smp, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp)) < 0) {
			perror(" line 85, sender fread error");
			exit(-1);
		}
		
		/* Send a message of type SENDER_DATA_TYPE */
 		msg.mtype = SENDER_DATA_TYPE;
 		if (msgsnd(msqid, &msg, sizeof(int), 0) == -1) {
			perror(" line 92, sender msgsnd error");
			exit(1);
		}
		
		/* Wait until the receiver sends us a message of type RECV_DONE_TYPE. */
 		if (msgrcv(msqid, &msg, sizeof(int), RECV_DONE_TYPE, 0) == -1) {
			perror(" line 98, sender msgrcv error");
			exit(1);
        }
	}

	/* Send a message of type SENDER_DATA_TYPE with size=0 */
	msg.mtype = SENDER_DATA_TYPE;
	msg.size = 0;
	if (msgsnd(msqid, &msg, sizeof(int), 0) == -1) {
		perror(" line 107, sender msgsnd error");
		exit(1);
	}
		
	/* Close the file */
	fclose(fp);
}// end of send


/* Main */
int main(int argc, char** argv)
{
	/* Check the command line arguments */
	if (argc < 2)
	{
		fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
		exit(-1);
	}

	init();
	send(argv[1]);
	cleanUp();
		
	return 0;
}
