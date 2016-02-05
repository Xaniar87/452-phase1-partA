#include "phase1.h"
#include "stdlib.h"
#include "stdio.h"


int func(void *arg){
  USLOSS_Console("Running func!!\n");
  return 0;
}


int P2_Startup(void *notused) 
{
    char *string = malloc(50);
    sprintf(string,"Process #%d",1);
    int i = P1_Fork(string, &func, NULL, USLOSS_MIN_STACK, 1);
    sprintf(string,"Process #%d",2);
    int j = P1_Fork(string, &func, NULL, USLOSS_MIN_STACK, 2);
    sprintf(string,"Process #%d",3);
    int a = P1_Fork(string, &func, NULL, USLOSS_MIN_STACK, 3);
    sprintf(string,"Process #%d",4);
    int b = P1_Fork(string, &func, NULL, USLOSS_MIN_STACK, 4);
    sprintf(string,"Process #%d",5);
    int c = P1_Fork(string, &func, NULL, USLOSS_MIN_STACK, 5);
    printf("Parent = %d, Children = %d,%d,%d,%d,%d\n",P1_GetPID(),i,j,a,b,c);
    /*Each Process should have a unique name in dump process*/
    /*Depending on how you implemented your dispatcher Process 1 Might be done already*/
    P1_DumpProcesses();
    return 0;
}

void setup(void) {
    // Do nothing.
}

void cleanup(void) {
    // Do nothing.
}
