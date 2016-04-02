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

//int Sys_TermRead(char *buff, int bsize, int unit, int *nread)
 
int P3_Startup(void *arg) {
	char buffer[200];
	int bytes = 0;
	Sys_Sleep(10);
	Sys_Sleep(10);
	while(1){
		Sys_TermRead(buffer,200,1,&bytes);
		USLOSS_Console("bytes = %d %s",bytes,buffer);
	}
	return 7;
}
 
void setup(void) {
    // Do nothing.
}
 
void cleanup(void) {
    // Do nothing.
}
