#include "phase1.h"
#include <stdio.h>
#include <stdlib.h>


P1_Semaphore sem;

int Child(void *arg){
	USLOSS_Console("In Child\n");
	P1_P(sem);
	int i = 0;
	//USLOSS_Console("%d\n",P1_WaitDevice(USLOSS_CLOCK_DEV,0,&i));
        //USLOSS_Console("%d\n",P1_WaitDevice(USLOSS_CLOCK_DEV,0,&i));
	USLOSS_Console("Just woken up\n");
	return 0;
}
 
int P2_Startup(void *notused) 
{
	int i = 1;
	sem = P1_SemCreate(0);
        P1_Fork("Child",&Child,NULL,USLOSS_MIN_STACK,2);
	P1_Join(&i);
	USLOSS_Console("Done\n");
    	return 0;
}
 
void setup(void) {
   // do nothing
}
 
void cleanup(void) {
    // Do nothing.
}
