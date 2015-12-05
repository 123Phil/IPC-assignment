// Phillip Stewart
// CPSC 351, Spring 2015

#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"


/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The name of the received file */
const char recvFileName[] = "recvfile";
FILE* fp;

/* Shared memory identifier, message queue identifier, shared memory pointer */
int shmid = 0;
int msqid = 0;
void* smp = 0;


/* Sets up the shared memory segment and message queue */
void init()
{
	/* Use ftok in order to generate a key. */
	int key;
	if ((key = ftok("keyfile.txt", 'a')) == -1) { 
        perror(" line 32, recv ftok error");
        exit(1);
    }

	/* Get the id of the shared memory segment of size SHARED_MEMORY_CHUNK_SIZE */
	if ((shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0666 | IPC_CREAT)) == -1) {
        perror(" line 38, recv shmget error");
        exit(1);
    }
	
	/* Attach to the shared memory */
    if ((int)(smp = shmat(shmid, NULL, 0)) == -1) {
        perror(" line 44, recv shmat error");
        exit(1);
    }
	
	/* Attach to the message queue */
	if ((msqid = msgget(key, 0666 | IPC_CREAT)) == -1) {
        perror(" line 50, recv msgget error");
        exit(1);
    }
}
 

/* The main loop for receiving data */
void receive()
{
	/* Open the file for writing */
	fp = fopen(recvFileName, "w");
	/* Error checks */
	if(!fp)
	{
		perror(" line 64, recv fopen error");	
		exit(-1);
	}
		
    /* Receive the first message and get the message size. */
	message msg;
	if (msgrcv(msqid, &msg, sizeof(int), SENDER_DATA_TYPE, 0) == -1) {
		perror(" line 71, recv msgrcv error");
		exit(1);
	}
	
	/* Loop and receive until the sender sets the size to 0, indicating that
	 * there is no more data to send */	
	int bytes_written;
	while(msg.size > 0) {
		/* Save the shared memory to file */
		bytes_written = fwrite(smp, sizeof(char), msg.size, fp);
		if(bytes_written != msg.size) {
			perror(" line 82, recv fwrite error");
			exit(1);
		}
			
		/* Message sender with type RECV_DONE_TYPE */
		msg.mtype = RECV_DONE_TYPE;
 		if (msgsnd(msqid, &msg, sizeof(int), 0) == -1) {
			perror(" line 89, recv msgsnd error");
			exit(1);
		}
		
		/* Receive the next message and size. */
		if (msgrcv(msqid, &msg, sizeof(int), SENDER_DATA_TYPE, 0) == -1) {
			perror(" line 95, recv msgrcv error");
			exit(1);
		}
	}
	/* Close the file */
	fclose(fp);
}


/* Perfoms the cleanup functions */
void cleanUp()
{
	/* Detach from shared memory */
	if (smp)
	if (shmdt(smp) == -1) {
        perror(" line 110, recv shmdt error");
        exit(1);
    }
    
    if (shmid)
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror(" line 116, recv shmctl error");
        exit(1);
    }
    
    if (msqid)
    if (msgctl(msqid, IPC_RMID, NULL) == -1) {
        perror(" line 122, recv msgctl error");
        exit(1);
    }
    
    if (fp)
    fclose(fp);
}


/* Handles the exit signal
 * @param signal - the signal type */
void ctrlCSignal(int signal)
{
	cleanUp();
	exit(130); //exit code for ^C
}


/* Main */
int main(int argc, char** argv)
{
	/* Set signal handler for ^C */
	signal(SIGINT, ctrlCSignal);
	
	init();
	receive();
	cleanUp();
	
	return 0;
}
