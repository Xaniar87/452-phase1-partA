#include <string.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <assert.h>
#include "libuser.h"

int worker(void *arg) {
	int mb = (int)arg;
	char buff[20];
	int size = 20;

	// place in an infinite receive...
	Sys_MboxReceive(mb, buff, &size);
	return size;
}

int P3_Startup(void *arg) {
	int mb;
	int succ;

	USLOSS_Console("creating a mailbox...\n");
	
	succ = Sys_MboxCreate(1, 5, &mb);

	assert(mb != -1);
	assert(succ == 0);

	USLOSS_Console("Releaseing mailbox...\n");

	succ = Sys_MboxRelease(mb);
	assert(succ == 0);
	USLOSS_Console("Passed!\n");

	USLOSS_Console("Release an invalid mailbox...\n");
	succ = Sys_MboxRelease(34973);
	assert(succ == -1);
	succ = Sys_MboxRelease(-3490734);
	assert(succ == -1);
	succ = Sys_MboxRelease(42);
	assert(succ == -1);
	USLOSS_Console("Passed!\n");

	USLOSS_Console("Release a mailbox already released...\n");
	succ = Sys_MboxRelease(mb);
	assert(succ == -1);
	USLOSS_Console("Passed!\n");

	USLOSS_Console("creating a mailbox...\n");
	succ = Sys_MboxCreate(1, 5, &mb);
	assert(mb != -1);
	assert(succ == 0);

	USLOSS_Console("creating a worker to wait for a receive...\n");

	int pid;
	Sys_Spawn("worker", worker, (void *)mb, USLOSS_MIN_STACK, 2, &pid);
	
	USLOSS_Console("Attempting to destroy the mailbox with the worker waiting on it...\n");
	Sys_MboxRelease(mb);


	return 0;

}


void setup(void) {
	// do nothing
}

void cleanup(void) {
	// do nothing
}
