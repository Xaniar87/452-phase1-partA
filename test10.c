#include "phase1.h"
#include "stdlib.h"
#include "stdio.h"


int num_procs_created = 5;
int even_or_odd = 0;
/* Tests killing processes*/

int runner(void *notused){
   if(P1_GetState(P1_GetPID()) == 2){
        return 0;
   }


   USLOSS_Console("Past kill check\n");
   return 0;
}

int P2_Startup(void *notused) 
{
    int pid1 = P1_Fork("process",&runner,NULL,USLOSS_MIN_STACK,1);
    P1_Fork("process",&runner,NULL,USLOSS_MIN_STACK,1);
    P1_Kill(pid1);
    P1_DumpProcesses();
    return 0;
}

void setup(void) {
    // Do nothing.
}

void cleanup(void) {
    // Do nothing.
}
