/*
 * simpleClockDriverTest.c
 *
 *  Created on: Mar 9, 2015
 *      Author: tishihara
 */
 
#include <string.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <assert.h>
#include <libuser.h>
 
int a(void *arg){
    USLOSS_Console("Started..\n");
    int initTime;
    Sys_GetTimeofDay(&initTime);
    Sys_Sleep(100);
    int finalTime;
    Sys_GetTimeofDay(&finalTime);
    USLOSS_Console("Total time: %f seconds\n", (finalTime-initTime)/1000000.0);
    USLOSS_Console("Woke up!\n");
    USLOSS_Console("You passed the test! Treat yourself to a cookie!\n");
    return 18247;
}

int P3_Startup(void *arg) {
    int i = 0;
    int j = 0;
    while(i >= 0){
	Sys_Spawn("a",a,NULL,USLOSS_MIN_STACK,4,&i);
    }
    USLOSS_Console("Here\n");
    while(Sys_Wait(&i,&j) != -1){
    	USLOSS_Console("Waiting %d %d\n",i,j);
    }
    return 7;
}
 
void setup(void) {
    // Do nothing.
}
 
void cleanup(void) {
    // Do nothing.
}
