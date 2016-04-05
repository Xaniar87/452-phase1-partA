/*
 * simpleWriteAndReadFromDiskTest.c
 *
 *  Created on: Mar 8, 2015
 *      Author: jeremy
 */
 
#include <string.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <assert.h>
#include <libuser.h>
 
int P3_Startup(void *arg) {
    USLOSS_Console("This test assumes that following:\n");
    USLOSS_Console("\tDisk 0 has 10 tracks\n");
    int *buffer = calloc(1, USLOSS_DISK_SECTOR_SIZE);
    buffer[0] = 42;
    int status;
    USLOSS_Console("Write to the disk\n");
    int result = Sys_DiskWrite(buffer, 0, 0, 1, 0, &status);
    USLOSS_Console("Verify that the disk send was succesful\n");
    assert(result == 0);
    assert(status == 0);
    free(buffer);
    buffer = calloc(1, USLOSS_DISK_SECTOR_SIZE);
    status = -12;
    USLOSS_Console("Read from the disk\n");
    result = Sys_DiskRead(buffer, 0, 0, 1, 0, &status);
    USLOSS_Console("Verify that the disk read was succesful\n");
    assert(result == 0);
    assert(status == 0);
    assert(buffer[0] == 42);
    USLOSS_Console("You passed all the tests! Treat yourself to a cookie!\n");
    return 7;
}
 
void setup(void) {
    system("~jhh/452-students/usloss/makedisk/makedisk 0 10");
}
 
void cleanup(void) {
    system("rm disk0");
}
