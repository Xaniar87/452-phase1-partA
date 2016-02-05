#include "phase1.h"
#include <assert.h>
#include <stdio.h>
 
int Grandchild(void *arg) {
    USLOSS_Console("Grandchild\n");
    return 0;
}
 
int Child(void *arg) {
    int     pid;
    int     state;
    int     status = 0;
    USLOSS_Console("Child\n");
    pid = P1_Fork("Grandchild", Grandchild, NULL, USLOSS_MIN_STACK, 2);
    if (pid < 0) {
        USLOSS_Console("Unable to fork child: %d\n", pid);
        status = 1;
    } else {
        /*
         * Grandchild runs at priority 2, which is higher than ours. Part A is
         * run-to-completion, the grandchild should already have quit before
         * we get here.
         */
        state = P1_GetState(pid);
        if (state != 3) { // grandchild should have quit
            USLOSS_Console("Grandchild is in invalid state: %d\n", state);
            status = 1;
        }
    }
    return status;
}
 
int P2_Startup(void *notused) 
{
    int pid;
    int status = 0;
 
    USLOSS_Console("P2_Startup\n");
    pid = P1_Fork("Child", Child, NULL, USLOSS_MIN_STACK, 3);
    if (pid < 0) {
        USLOSS_Console("Unable to fork child: %d\n", pid);
        status = 1;
    }
    return status;
}
 
void setup(void) {
    // Do nothing.
}
 
void cleanup(void) {
    // Do nothing.
}
