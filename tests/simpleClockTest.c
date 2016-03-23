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
 
int P3_Startup(void *arg) {
    USLOSS_Console("Testing one call to Sys_Sleep\n");
    USLOSS_Console("Sleeping for 3 seconds...\n");
    int initTime;
    Sys_GetTimeofDay(&initTime);
    Sys_Sleep(3);
    int finalTime;
    Sys_GetTimeofDay(&finalTime);
    USLOSS_Console("Total time: %f seconds\n", (finalTime-initTime)/1000000.0);
    USLOSS_Console("Woke up!\n");
    USLOSS_Console("You passed the test! Treat yourself to a cookie!\n");
    return 7;
}
 
void setup(void) {
    // Do nothing.
}
 
void cleanup(void) {
    // Do nothing.
}
