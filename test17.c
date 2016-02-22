#include "phase1.h"
#include <assert.h>
#include <stdio.h>


int Child(void *arg) {
	printf("In child\n");
	USLOSS_WaitInt();
	USLOSS_WaitInt();
	USLOSS_WaitInt();
	USLOSS_WaitInt();
	USLOSS_WaitInt();
	USLOSS_WaitInt();
	USLOSS_WaitInt();
	USLOSS_WaitInt();
	printf("After wait\n");
	return 0;
}


int Child2(void *arg){
	printf("Child2\n");
	return 0;
}

 
int P2_Startup(void *notused) 
{
	P1_Fork("hey",&Child,NULL,USLOSS_MIN_STACK,2);
	P1_Fork("hey2",&Child2,NULL,USLOSS_MIN_STACK,2);
    	return 0;
}
 
void setup(void) {
    // Do nothing.
}
 
void cleanup(void) {
    // Do nothing.
}
