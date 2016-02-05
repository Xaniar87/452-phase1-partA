#include "phase1.h"
#include <stdio.h>
#include <stdlib.h>
 
// This test checks trying to kill processes which have invalid process ids and should return an error
 
int P3_Startup(void *notused);
 
int P2_Startup(void *notused) 
{
    USLOSS_Console("P2_Started\n");
    int result = P1_Kill(-1, 1);
    if (result != -1) {
       USLOSS_Console("Fail 1: should have been an error");
       return 0;
    }
 
   
    result = P1_Kill(50, 1);
    if (result != -1) {
       USLOSS_Console("Fail 2: should have been an error");
       return 0;
    }
 
 
    result = P1_GetState(-1);
    if (result != -1) {
       USLOSS_Console("Fail 3: should have been an error");
       return 0;
    }
 
    result = P1_GetState(50);
    if (result != -1) {
       USLOSS_Console("Fail 4: should have been an error");
       return 0;
    }
 
     
    P1_Kill(42,1);
    result = P1_GetState(42); 
    if (result != 2) {
       USLOSS_Console("Fail 5: state of process 42 should be marked for death");
       return 0;
    }
    
    result = P1_GetState(0);
    if (result != 1) {
        USLOSS_Console("Fail 6: sentinel should be in ready state");
        return 0;
    }
 
    USLOSS_Console("PASS\n");
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
