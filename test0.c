#include "phase1.h"
#include "stdlib.h"
#include "stdio.h"

int func2(void *arg);

int func(void *arg){
  USLOSS_Console("Running func!!\n");
  return 0;
}

int func2(void *arg){
  USLOSS_Console("Running func2!!\n");
  P1_Kill(1,0);
  P1_DumpProcesses();
  return 0;
}

int P2_Startup(void *notused) 
{
    //printf("%d\n",P1_Kill(P1_GetPID() - 100,0));
    int i = P1_Fork("hi", &func, NULL, USLOSS_MIN_STACK, 1);
    int j = P1_Fork("hi", &func2, NULL, USLOSS_MIN_STACK, 0);
    printf("%d %d\n",i,j);
    printf("%d\n",P1_GetState(P1_GetPID()));
    if(P1_GetState(P1_GetPID()) == 5){
      return 1;
    }
    USLOSS_Console("P2_Startup\n");
    return 0;
}

void setup(void) {
    // Do nothing.
}

void cleanup(void) {
    // Do nothing.
}
