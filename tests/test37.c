/*
 * simpleOneReaderOneWriterDiskTest.c
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
 
int reading0Done;
int reading1Done;
int canRead0;
int canRead1;
 
int reader(void *arg) {
    int unit = *((int *)&arg);
    int bytes;
    int sectors;
    int tracks;
    int result = Sys_DiskSize(unit, &bytes, &sectors, &tracks);
    int currentTrack = 0;
    int *buffer = calloc(1, USLOSS_DISK_TRACK_SIZE * USLOSS_DISK_SECTOR_SIZE);
    int i;
    int status;
    while (currentTrack < tracks) {
        // Clear the buffer to ensure a fresh test
        memset(buffer, 0, USLOSS_DISK_TRACK_SIZE * USLOSS_DISK_SECTOR_SIZE);
        // Read every track and confirm that it is correct
        // Wait until the track has been written
        Sys_SemP(canRead0);
        USLOSS_Console("Starting a read for unit: %d\n", unit);
        result = Sys_DiskRead(buffer, currentTrack, 0, USLOSS_DISK_TRACK_SIZE, unit, &status);
        USLOSS_Console("Ending a read for unit: %d\n", unit);
        assert(result == 0);
        assert(status == 0);
        for (i = 0; i < (USLOSS_DISK_TRACK_SIZE * USLOSS_DISK_SECTOR_SIZE)/4; i++) {
            assert(buffer[i] == i);
        }
        currentTrack++;
        // Tell the writer that we are done
        Sys_SemV(reading0Done);
    }
    return 0;
 
}
 
int writer(void *arg) {
    int unit = *((int *)&arg);
    int bytes;
    int sectors;
    int tracks;
    int result = Sys_DiskSize(unit, &bytes, &sectors, &tracks);
    assert(result == 0);
    int currentTrack = 0;
    int *buffer = calloc(1, USLOSS_DISK_TRACK_SIZE * USLOSS_DISK_SECTOR_SIZE);
    int i = 0;
    int status;
 
    for (i = 0; i < (USLOSS_DISK_TRACK_SIZE * USLOSS_DISK_SECTOR_SIZE)/4; i++) {
        buffer[i] = i;
    }
    // Go for every track in the disk
    while (currentTrack < tracks) {
        USLOSS_Console("Starting a write for unit: %d\n", unit);
        result = Sys_DiskWrite(buffer, currentTrack, 0, USLOSS_DISK_TRACK_SIZE, unit, &status);
        USLOSS_Console("Ending a write for unit: %d\n", unit);
        assert(result == 0);
        assert(status == 0);
        currentTrack++;
 
        // Wait for all the readers to finish reading
        if (unit == 0) {
            // Indicate that the readers can read
 
            Sys_SemV(canRead0);
            // Wait for all the readers to finish reading
            // Wait for 25 times
            Sys_SemP(reading0Done);
            USLOSS_Console("Readers done.\n");
        }
    }
    return 0;
 
 
}
 
int P3_Startup(void *arg) {
    int result;
    int pid;
    int i;
    USLOSS_Console("WANRING: this test assumes you have the base functionality down, if you don't do that first and come back\n");
    USLOSS_Console("This test assumes that following:\n");
    USLOSS_Console("\tDisk 0 has 10 tracks\n");
    result = Sys_Spawn("Writer for disk1", writer, (void *)0, USLOSS_MIN_STACK, 4, &pid);
    assert(result == 0);
    assert(pid >= 0);
    result = Sys_SemCreate(0, &reading0Done);
    assert(result == 0);
    result = Sys_SemCreate(0, &canRead0);
    assert(result == 0);
    result = Sys_Spawn("Reader for disk 0", reader, (void *)0, USLOSS_MIN_STACK, 5, &pid);
    assert(result == 0);
    assert(pid >= 0);
    // Wait 2 times for everything to finish
    for (i = 0; i < 2; i++) {
        Sys_Wait(&pid, &result);
    }
    return 7;
}
 
void setup(void) {
    // Do nothing.
    system("~jhh/452-students/usloss/makedisk/makedisk 0 10");
}
 
void cleanup(void) {
    system("rm disk0");
}
