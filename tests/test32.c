#include "phase2.h"
#include <assert.h>
#include <usloss.h>
#include <stdlib.h>
#include <stdarg.h>
#include <phase1.h>
#include <stdio.h>
#include "libuser.h" 
/*
test30.c will be testing Sys_Spawn to see if the P1_Fork function is correctly forking
new processes
*/

int count = 0; 
int P3_Startup(void *notused);
int P4_Startup(void *notused);
int P5_Startup(void *notused);
int P6_Startup(void *notused);
int P7_Startup(void *notused);
int P8_Startup(void *notused);
P1_Semaphore sem;

 
int P3_Startup(void *notused) 
{
    	int pid;
	USLOSS_Console("P3_Startup\n");
    	int result = Sys_Spawn("P4_Startup", P4_Startup, NULL, 4 *  USLOSS_MIN_STACK, 3, &pid);
	printf("P3 pid changedi(P4 pid): %d\n", pid);
	assert(result == 0);
	Sys_DumpProcesses();
	USLOSS_Console("P3_Finished\n");
	return 0; 
}
 
int P4_Startup(void *notused) {
    	int pid;
	USLOSS_Console("P4_Startup\n");
	int result = Sys_Spawn("P5_Startup", P5_Startup, NULL, 4 *  USLOSS_MIN_STACK, 4, &pid);
        printf("P4 pid changed: %d\n", pid);
    	USLOSS_Console("P4_Finished\n");
   	return 0;
}


int P5_Startup(void *notused)
{
	int pid;
    	USLOSS_Console("P5_Startup\n");
 	int result = Sys_Spawn("P6_Startup", P6_Startup, NULL, 4 *  USLOSS_MIN_STACK, 2, &pid);
        printf("P5 pid changed: %d\n", pid);
	Sys_DumpProcesses();
	USLOSS_Console("P5_Finished\n");
   	return 0;
}


int P6_Startup(void *notused)
{
    int pid;
    USLOSS_Console("P6_Startup\n");
    int result = Sys_Spawn("P7_Startup", P7_Startup, NULL, 4 *  USLOSS_MIN_STACK, 5, &pid);
    printf("P6 pid changed: %d\n", pid);
    assert(result == 0);
    int status = 4;
    Sys_Terminate(status);
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
