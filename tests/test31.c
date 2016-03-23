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
int sem;

 
int P3_Startup(void *notused) 
{
        USLOSS_Console("%d\n",Sys_SemCreate(0,&sem));
	USLOSS_Console("%d\n",Sys_SemV(sem));
	USLOSS_Console("%d\n",Sys_SemP(sem));
	USLOSS_Console("%d\n",Sys_SemFree(sem));
	USLOSS_Console("Past\n");
	Sys_DumpProcesses();
	return 0; 
}
 
void setup(void) {
   // do nothing
}
 
void cleanup(void) {
    // Do nothing.
}
