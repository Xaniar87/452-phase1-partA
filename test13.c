#include "phase1.h"
#include <assert.h>
#include <stdio.h>


int num = 0; 
int Child(void *arg) {
    if(num > 1000){
	return 1;
    }
    int i;
    for(i = 0; i < P1_MAXPROC;i++){
        if(P1_Fork("hey",&Child,NULL,USLOSS_MIN_STACK,3) >= 0){
		num++;
                USLOSS_Console("Hey\n");
	}
    }
    if(P1_GetState(P1_GetPID() == 2)){
        return 1;
    }
    USLOSS_Console("Here");
    return 0;
}
 
int P2_Startup(void *notused) 
{
    int i;
    for(i = 0; i < P1_MAXPROC;i++){
	P1_Fork("hey",&Child,NULL,USLOSS_MIN_STACK,2);
    }
    for(i = 1; i < P1_MAXPROC;i++){
	P1_Kill(i);
    }
    return 0;
}
 
void setup(void) {
    // Do nothing.
}
 
void cleanup(void) {
    // Do nothing.
}
