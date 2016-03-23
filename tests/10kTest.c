#include <string.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <assert.h>
#include "libuser.h"

/* creates and releases 10k mailboxes.. modeled after the fork_10k test */

int P3_Startup(void *arg) {
	int succ;
	int mb;
	
	
	USLOSS_Console("Creating 10,000 Mailboxes and deleting them as we go...\n");	

	int i;
	for (i = 0; i < 10000; i++) {
		succ = Sys_MboxCreate(1, 5, &mb);
		assert(mb != -1);
		assert(succ == 0);

		succ = Sys_MboxRelease(mb);
		assert(succ == 0);
	}

	USLOSS_Console("Passed! .. cookie time!\n");
	

	return 0;

}


void setup(void) {
	// do nothing
}

void cleanup(void) {
	// do nothing
}
