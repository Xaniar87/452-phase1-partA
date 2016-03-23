#include <string.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <assert.h>
#include "libuser.h"

int P3_Startup(void *arg) {
	USLOSS_Console("creating a mailbox...\n");
	int mb;
	int succ;
	succ = Sys_MboxCreate(0, 0, &mb);

	assert(mb == -1);
	assert(succ == -1);

	succ = Sys_MboxCreate(1, 5, &mb);

	assert(mb != -1);
	assert(succ == 0);
	
	int i;
	for (i = 0; i < 199; i++) {
		succ = Sys_MboxCreate(1, 5, &mb);

		assert(mb != -1);
		assert(succ == 0);
	}

	succ = Sys_MboxCreate(1, 5, &mb);

	assert(mb == -1);
	assert(succ == 0);
	

	return 0;

}


void setup(void) {
	// do nothing
}

void cleanup(void) {
	// do nothing
}
