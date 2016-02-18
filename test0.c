#include "phase1.h"
#include <stdio.h>
#include <stdlib.h>


P1_Semaphore sem;

int Child(void *arg){
	USLOSS_Console("In Child\n");
	P1_P(sem);
	USLOSS_Console("Just woken up\n");
	return 0;
}
 
int P2_Startup(void *notused) 
{
	//sem = P1_SemCreate(0);
	sem = malloc(5);
	P1_Fork("Child",&Child,NULL,USLOSS_MIN_STACK,0);
    	P1_DumpProcesses();
	P1_V(sem);
	USLOSS_Console("Done\n");
    	return 0;
}
 
void setup(void) {
   // do nothing
}
 
void cleanup(void) {
    // Do nothing.
}
