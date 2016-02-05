#include "phase1.h"
#include "stdlib.h"
#include "stdio.h"


/* Basic Test that tests passing an argument into the new process */

int print_int(void *i){
	int *num = (int *)i;
	char string[50] = {0};
	sprintf(string,"Number = %d\n",*num);
	USLOSS_Console(string);
	return 0;
}

int P2_Startup(void *notused) 
{
    int *i = malloc(sizeof(int));
    *i = 10;
    /*Priority 0 Process. Should complete before P2 Startup completes*/
    P1_Fork("Number Printer 1",&print_int,(void *)i,USLOSS_MIN_STACK,0);
    *i = 15;
    /*Priority 5 Process. Should finish AFTER P2_Startup*/
    P1_Fork("Number Printer 2",&print_int,(void *)i,USLOSS_MIN_STACK,5);
    USLOSS_Console("P2_Startup done\n");
    return 0;
}

void setup(void) {
    // Do nothing.
}

void cleanup(void) {
    // Do nothing.
}
