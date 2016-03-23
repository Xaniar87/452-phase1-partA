#include <string.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <assert.h>
#include "libuser.h"

// this will test your mailbox blocking.  It's written to take time by basically busy waiting.

int worker(void *arg) {
	int i;
	int mb = (int)arg;
	char *hello = "hello";
	int size = strlen(hello) + 1;
	Sys_MboxSend(mb, hello, &size);
	USLOSS_Console("size: %d\n", size);


	for (i = 0; i < 1000000000; i++) {
		if (i % 100000000 == 0) {
			Sys_MboxSend(mb, hello, &size);
		}
	}

	return 0;
}

int worker2(void *arg) {
	int mb = (int)arg;
	char buffer[20];
	int size = 20;
	while (1) {
		Sys_MboxReceive(mb, buffer, &size);
		USLOSS_Console("receieved %s\n", buffer);		
	}

	return 0;
}


int P3_Startup(void *arg) {
	USLOSS_Console("creating a mailbox...");
	int mb;
	int succ;
	succ = Sys_MboxCreate(0, 0, &mb);

	assert(mb == -1);
	assert(succ == -1);

	succ = Sys_MboxCreate(10, 50, &mb);

	assert(mb != -1);
	assert(succ == 0);

	int pid;
	Sys_Spawn("worker", worker, (void *)mb, USLOSS_MIN_STACK, 2, &pid);
	Sys_Spawn("worker2", worker2, (void *)mb, USLOSS_MIN_STACK, 2, &pid);

	USLOSS_Console("Congrats! Hit Ctrl-C to continue...\n");

	while (1);  // keep the process going prevents deadlock.  User must exit..

	return 0;

}


void setup(void) {
	// do nothing
}

void cleanup(void) {
	// do nothing
}
