#include "phase1.h"
#include <stdio.h>
#include <stdlib.h>
 
// This test attempts to fork a process with an invalid priority
// The Test should print "P2_Startup" and then return "-3" when a fork of a process with an invalid priority is attempted.
// Lastly it should dump processes with the two valid processes, sentinel and P2_Startup. 
 
int P3_Startup(void *notused);
 
int P2_Startup(void *notused) 
{
    USLOSS_Console("P2_Startup\n");
    int result = P1_Fork("P3_Startup", P3_Startup, NULL, 4 * USLOSS_MIN_STACK, -1);
    if (result != -3)
        USLOSS_Console("Fail: invalid priority should return -3");
    
    P1_DumpProcesses();
    return 0;
}
 
int P3_Startup(void *notused) {
    USLOSS_Console("P3_Startup\n");
    return 0;
}
 
 
void setup(void) {
   // do nothing
}
 
void cleanup(void) {
    // Do nothing.
}
