#include "phase1.h"
#include <stdio.h>
#include <stdlib.h>
 
 

int count = 0; 
int P3_Startup(void *notused);
int P4_Startup(void *notused);
int P5_Startup(void *notused);
int P6_Startup(void *notused);
int P7_Startup(void *notused);
int P8_Startup(void *notused);
P1_Semaphore sem;

int P2_Startup(void *notused) 
{
	USLOSS_Console("P2 Startup\n");	
    	P1_Fork("P3_Startup", P3_Startup, NULL, 4 *  USLOSS_MIN_STACK, 1);
	USLOSS_Console("P2 finish\n");
    	return 0;
}
 
int P3_Startup(void *notused) 
{
    	USLOSS_Console("P3_Startup\n");
    	P1_Fork("P3_Startup", P3_Startup, NULL, 4 *  USLOSS_MIN_STACK, 3);
	USLOSS_Console("P3_Finished\n");
	return 0; 
}
 
int P4_Startup(void *notused) {
    	USLOSS_Console("P4_Startup\n");
    	USLOSS_Console("P4_Finished\n");
   	return 0;
}


int P5_Startup(void *notused)
{
    	USLOSS_Console("P5_Startup\n");
 	USLOSS_Console("P5_Finished\n");
   	return 0;
}


int P6_Startup(void *notused)
{
    USLOSS_Console("P6_Startup\n");
    USLOSS_Console("P6_Finished\n");
    return 0;
}


int P7_Startup(void *notused)
{
    USLOSS_Console("P7_Startup\n");
    USLOSS_Console("P7_Finished\n");
    return 0;
}


int P8_Startup(void *notused)
{
    USLOSS_Console("P8_Startup\n");
    USLOSS_Console("P8_Finished\n");
    return 0;
}
 
void setup(void) {
   // do nothing
}
 
void cleanup(void) {
    // Do nothing.
}
