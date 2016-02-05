#include "phase1.h"
#include "stdlib.h"
#include "stdio.h"


/* Test the limits of process creation.*/

int print_hello(void *notused){
   USLOSS_Console("Hello\n");
   return 0;
}

int P2_Startup(void *notused) 
{
    while(P1_Fork("process",&print_hello,NULL,USLOSS_MIN_STACK,5) >= 0);
    P1_DumpProcesses();
    return 0;
}

void setup(void) {
    // Do nothing.
}

void cleanup(void) {
    // Do nothing.
}
