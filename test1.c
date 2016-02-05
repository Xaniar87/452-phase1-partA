#include "phase1.h"
#include <stdio.h>
#include <stdlib.h>
 
// This test attempts to fork a process with invalid stack size.
// Should only run P2_Startup and sentinel.
// Prints "P2_Startup" then should return -2 then dumps the only 2 valid processes
 
int P3_Startup(void *notused);
 
int P2_Startup(void *notused) 
{
    USLOSS_Console("P2_Startup\n");
 
    int result  = P1_Fork("P3_Startup", P3_Startup, NULL, 1, 2);
    
    if (result != -2)
        USLOSS_Console("Fail: should return -2 with invalid stack size");
    
    P1_DumpProcesses();
 
    return 0;
}
 
int P3_Startup(void *notused) 
{
    USLOSS_Console("P3_Startup\n");
    return 0; 
}
 
void setup(void) {
   // do nothing
}
 
void cleanup(void) {
    // Do nothing.
}
