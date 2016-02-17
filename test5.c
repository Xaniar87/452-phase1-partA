#include "phase1.h"
#include <stdio.h>
#include <stdlib.h>
 
// This test checks that the current pid in P2_Startup is 1 and that the current state of pid 1 is running
// Also it checks that the state of the sentinel is ready
// Finally it attempts to kill itself which should return an error of -2 since you cant call 
// P1_Kill on the currently running process
// Correct output:
 
/*
 * P2_Startup
 * PASS
 * P2_Finished
 * Goodbye.
*/
 
int P3_Startup(void *notused);
 
int P2_Startup(void *notused) 
{
    USLOSS_Console("P2_Startup\n");
      
    if (P1_GetPID() != 1) {
        USLOSS_Console("Fail: id does not equal 1");
    }
 
    if (P1_GetState(1) != 0) {
        USLOSS_Console("Fail: state is not running");
    }
    if (P1_GetState(0) != 1) {
        USLOSS_Console("Fail: state of sentinel should be ready");
    }
    int result = P1_Kill(P1_GetPID());
 
    if (result == -2) {
           USLOSS_Console("PASS\n");
    } 
     
    USLOSS_Console("P2_Finished\n");    
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
