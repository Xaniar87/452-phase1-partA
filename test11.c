#include "phase1.h"
#include <assert.h>
#include <stdio.h>
 
int Child(void *arg) {
    USLOSS_Console("Child\n");
    return 0;
}
 
int P2_Startup(void *notused) 
{
    int pid;
    int state;
    int status = 0;
 
    USLOSS_Console("P2_Startup\n");
    pid = P1_Fork("Child", Child, NULL, USLOSS_MIN_STACK, 3);
    if (pid < 0) {
        USLOSS_Console("Unable to fork child: %d\n", pid);
        status = 1;
    } else {
        /*
         * Child runs at priority 3, which is lower than ours. Part A is
         * run-to-completion, so we should continue to run while the 
         * child waits.
         */
 
        state = P1_GetState(pid);
        if (state != 1) { // child should be ready
            USLOSS_Console("Child is in invalid state: %d\n", state);
            status = 1;
        }
    }
    return status;
}
 
void setup(void) {
    // Do nothing.
}
 
void cleanup(void) {
    // Do nothing.
}
