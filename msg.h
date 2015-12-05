// Phillip Stewart
// CPSC 351, Spring 2015

/* The information type */ 
#define SENDER_DATA_TYPE 1

/* The done message */
#define RECV_DONE_TYPE 2

/* The message structure */
typedef struct _message
{
	/* The message type */
	long mtype;
	
	/* How many bytes in the message */
	int size;
} message;
