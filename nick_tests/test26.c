#include "phase1.h"
#include <stdio.h>
#include <stdlib.h>


P1_Semaphore sem;

int P3_Startup(void *notused);

int P2_Startup(void *notused) 
{
    USLOSS_Console("P2_Started\n"); 
    sem = P1_SemCreate(0); 
    P1_Fork("P3_Startup", P3_Startup, NULL, 4 *  USLOSS_MIN_STACK, 1);
    int x = 200;
    int* status = &x;
    USLOSS_DeviceOutput(USLOSS_ALARM_DEV,0,status); 
    P1_WaitDevice(1, 0, status); 
    P1_V(sem);
    P1_DumpProcesses();
    USLOSS_Console("P2 Finished\n");
    return 0;
}

int P3_Startup(void *notused) {
    USLOSS_Console("P3_Started");
    P1_P(sem);
    USLOSS_Console("P3_Finished\n");
    return 0;
}


void setup(void) {
   // do nothing
}

void cleanup(void) {
    // Do nothing.
}
