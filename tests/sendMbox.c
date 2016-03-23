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

	succ = Sys_MboxCreate(1, 10, &mb);

	assert(mb != -1);
	assert(succ == 0);

	char *hello = "hello";
	int size = strlen(hello) + 1;

	USLOSS_Console("sending a message %s to mailbox\n", hello);
	
	/* send 1 */
	succ = Sys_MboxSend(mb, (void*)hello, &size);
	assert(succ == 0);

	char buffer[20];
	int msgSize = 20;

	USLOSS_Console("Attempting to receive message...\n");
	succ = Sys_MboxReceive(mb, (void*)buffer, &msgSize);
	assert(succ == 0);

	USLOSS_Console("The message size was %d\n", msgSize);
	USLOSS_Console("the message has arrived! it is... %s\n", buffer);

	USLOSS_Console("sending a message that is too long to mailbox\n");

	char *hello2 = "hello this is a UUUUGE message.. donald trump style";
	int size2 = strlen(hello2) + 1;

	/* send 2 */
	succ = Sys_MboxSend(mb, (void*)hello2, &size2);
	assert(succ == -1);
	USLOSS_Console("Passed!\n");

	USLOSS_Console("Testing conditional send...\n");
	Sys_MboxCreate(1, 20, &mb);

	size = strlen(hello) + 1;
	succ = Sys_MboxCondSend(mb, (void*)hello, &size);
	assert(succ == 0);

	size = strlen(hello) + 1;
	succ = Sys_MboxCondSend(mb, (void*)hello, &size);
	assert(succ == 1);

	USLOSS_Console("Passed!\n");

	USLOSS_Console("Testing Conditional Receive...\n");

	succ = Sys_MboxCondReceive(mb, (void*)buffer, &msgSize);
	assert(succ == 0);

	USLOSS_Console("First message received is size %d and says \"%s\"", msgSize, buffer);

	succ = Sys_MboxCondReceive(mb, (void*)buffer, &msgSize);
	assert(succ == 1);

	USLOSS_Console("Passed!\n");


	int i = 5;
	int x;
	int size4 = sizeof(int);
	succ = Sys_MboxCreate(50, size4, &mb);

	/* send 3 */
	Sys_MboxSend(mb, &i, &size4);
	Sys_MboxReceive(mb, &x, &size4);

	USLOSS_Console("received %d\n", x);

	return 0;

}


void setup(void) {
	// do nothing
}

void cleanup(void) {
	// do nothing
}
