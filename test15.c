#include "phase1.h"
#include <stdio.h>
#include <stdlib.h>


int Child(void *arg){
	printf("HERE\n");
	USLOSS_Console("In Child\n");
	return 919;
}
 
int P2_Startup(void *notused) 
{
	P1_Fork("Child",&Child,NULL,USLOSS_MIN_STACK,2);
	int status = 1;
	int p = P1_Join(&status);
	USLOSS_Console("%d\n",P1_GetPID());
	USLOSS_Console("Done %d %d\n",status,p);
    	return 0;
}
 
void setup(void) {
   // do nothing
}
 
void cleanup(void) {
    // Do nothing.
}
