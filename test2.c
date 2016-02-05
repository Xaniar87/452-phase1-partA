#include "phase1.h"
#include <stdio.h>
#include <stdlib.h>
 
// This test attempts to fork more than P1_MAXPROC processes
// Prints -1 once the limit has been reached -> should be printed 5 times
// Then prints all details of all 50 processes where P2_Startup should have 48 children
 
 
int P3_Startup(void *notused);
 
int P2_Startup(void *notused) 
{
    USLOSS_Console("P2_Startup\n");
    int i;
    for (i = 0; i < (P1_MAXPROC - 2) + 5; i++) {
        int result = P1_Fork("P3_Startup", P3_Startup, NULL, 4 *  USLOSS_MIN_STACK, 2);
        if (result == -1) {
            USLOSS_Console("%d\n", result);
        }
    }
    
    P1_DumpProcesses();
 
    return 0;
}
 
int P3_Startup(void *notused) 
{
   // USLOSS_Console("P3_Startup\n");
    return 0; 
}
 
void setup(void) {
   // do nothing
}
 
void cleanup(void) {
    // Do nothing.
}
