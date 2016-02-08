#include "phase1.h"
#include <assert.h>
#include <stdio.h>

int C(void *arg){
	USLOSS_Console("Hi\n");
	return 0;
}

int Child(void *arg) {
	P1_DumpProcesses();
	P1_Fork("grandchild",&C,NULL,USLOSS_MIN_STACK,1);
	P1_DumpProcesses();
	return 0;
}
 
int P2_Startup(void *notused) 
{
    int p = P1_Fork("should be dead",&Child,NULL,USLOSS_MIN_STACK,2);
    P1_Kill(p);
    return 0;
}
 
void setup(void) {
    // Do nothing.
}
 
void cleanup(void) {
    // Do nothing.
}
