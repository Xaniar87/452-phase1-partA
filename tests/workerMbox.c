
#include <string.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <assert.h>
#include "libuser.h"
#include "string.h"

/* creates workers with a random amount of waiting times.  
   Times should roughly match their worker number... 
   sample correct output...
   ...
   Worker 320 sleeping for 320 seconds..
    Worker 557 sleeping for 557 seconds..
    Worker 57 Total time: 57.789749 seconds
    the buffer was received: hello this is the message
    Worker 58 Total time: 59.489548 seconds
    the buffer was received: hello this is the message
    Worker 135 Total time: 135.089947 seconds
	...

	There will be more or less entries depending how you set numProcs
*/

int worker(void *arg) {
	int mbox = (int)arg;
	int seed;
	int initTime;
	int finalTime;
	int size;

	Sys_GetTimeofDay(&seed);
	srand(seed);
	int n = rand() % 1000;
	char *message = "hello this is the message";
	
	size = strlen(message) + 1;
	int num = n;

	USLOSS_Console("Worker %d sleeping for %d seconds..\n", num, num);
	Sys_GetTimeofDay(&initTime);
	Sys_Sleep(num);
	Sys_GetTimeofDay(&finalTime);
	USLOSS_Console("Worker %d Total time: %f seconds\n", num, (finalTime - initTime) / 1000000.0);

	Sys_MboxSend(mbox, message, &size);
	
	return 0;
}

int P3_Startup(void *arg) {

	srand(1);
	char buff[50];
	int i;
	int pid;
	int mbox;
	int numProc = 25;
	int succ = Sys_MboxCreate(50, 50, &mbox);
	assert(succ == 0);

	for (i = 0; i < numProc; i++) {
		Sys_Spawn("worker", worker, (void *)mbox, USLOSS_MIN_STACK, 3, &pid);
	}

	Sys_DumpProcesses();
	int size = 50;
	i = 0;
	while (i<numProc) {
		Sys_MboxReceive(mbox, buff, &size);
		USLOSS_Console("the buffer was received: %s\n", buff);
		i++;
	}

	USLOSS_Console("All messages received!\n", buff);

	return 7;
}

void setup(void) {
}

void cleanup(void) {
}
