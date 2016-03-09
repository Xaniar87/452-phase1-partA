#include <stdlib.h>
#include <phase1.h>
#include <phase2.h>
#include "usyscall.h"

/* Structs */

typedef struct userArgStruct {
	int (*func)(void*);
	void *arg;
} userArgStruct;

/*Prototypes*/
static int ClockDriver(void *arg);

static int launch(void *arg);

void sysHandler(int type, void *arg);

extern int permissionCheck(void);
extern void interruptsOff(void);
extern void interruptsOn(void);
void kernelMode(void);
void userMode(void);

int P2_Startup(void *arg) {
	USLOSS_IntVec[USLOSS_SYSCALL_INT] = sysHandler;
	P1_Semaphore running;
	int status;
	int pid;
	int clockPID;

	/*
	 * Create clock device driver
	 */
	running = P1_SemCreate(0);
	clockPID = P1_Fork("Clock driver", ClockDriver, (void *) running,
	USLOSS_MIN_STACK, 2);
	if (clockPID == -1) {
		USLOSS_Console("Can't create clock driver\n");
	}
	/*
	 * Wait for the clock driver to start.
	 */
	P1_P(running);
	/*
	 * Create the other device drivers.
	 */
	// ...
	/*
	 * Create initial user-level process. You'll need to check error codes, etc. P2_Spawn
	 * and P2_Wait are assumed to be the kernel-level functions that implement the Spawn and
	 * Wait system calls, respectively (you can't invoke a system call from the kernel).
	 */
	pid = P2_Spawn("P3_Startup", P3_Startup, NULL, 4 * USLOSS_MIN_STACK, 3);
	pid = P2_Wait(&status);

	/*
	 * Kill the device drivers
	 */
	P1_Kill(clockPID);
	// ...

	/*
	 * Join with the device drivers.
	 */
	// ...
	return 0;
}

void sysHandler(int type,void *arg) {
	USLOSS_Sysargs *sysArgs = (USLOSS_Sysargs *) arg;
	int retVal = 0;
	int retVal2 = 0;
	interruptsOn();
	switch (sysArgs->number) {
	case SYS_TERMREAD:
		break;
	case SYS_TERMWRITE:
		break;
	case SYS_SPAWN:
		retVal = P2_Spawn(sysArgs->arg5, sysArgs->arg1, sysArgs->arg2,
				(int) sysArgs->arg3, (int) sysArgs->arg4);
		if (retVal == -3 || retVal == -2) {
			sysArgs->arg4 = (void *) -1;
		} else {
			sysArgs->arg1 = (void *) retVal;
			sysArgs->arg4 = (void *) 0;
		}
		break;
	case SYS_WAIT:
		retVal = P2_Wait(&retVal2);
		if(retVal == -1){
			sysArgs->arg1 = (void *)-1;
			sysArgs->arg4 = (void *)-1;
			sysArgs->arg2 = (void *)-1;
		}else{
			sysArgs->arg1 = (void *)retVal;
			sysArgs->arg2 = (void *)retVal2;
			sysArgs->arg4 = (void *)0;
		}
		break;
	case SYS_TERMINATE:
		P2_Terminate((int)sysArgs->arg1);
		break;
	case SYS_SLEEP:
		break;
	case SYS_DISKREAD:
		break;
	case SYS_DISKWRITE:
		break;
	case SYS_GETTIMEOFDAY:
		break;
	case SYS_CPUTIME:
		break;
	case SYS_DUMPPROCESSES:
		break;
	case SYS_GETPID:
		break;
	case SYS_SEMCREATE:
		break;
	case SYS_SEMP:
		break;
	case SYS_SEMV:
		break;
	case SYS_SEMFREE:
		break;
	case SYS_MBOXCREATE:
		break;
	case SYS_MBOXRELEASE:
		break;
	case SYS_MBOXSEND:
		break;
	case SYS_MBOXRECEIVE:
		break;
	case SYS_MBOXCONDSEND:
		break;
	case SYS_MBOXCONDRECEIVE:
		break;
	default:
		P1_Quit(1);
	}
}

int P2_Spawn(char *name, int (*func)(void*), void *arg, int stackSize,
		int priority) {
	if (permissionCheck()) {
		return -1;
	}
	userArgStruct* uas = malloc(sizeof(userArgStruct));
	uas->arg = arg;
	uas->func = func;
	int retVal = P1_Fork(name, launch, uas, stackSize, priority);
	return retVal;
}

int launch(void *arg) {
	userArgStruct *uas = (userArgStruct *) arg;
	interruptsOn();
	userMode();
	int rc = uas->func(uas->arg);
	free(uas);
	USLOSS_Sysargs *sysArgs = malloc(sizeof(USLOSS_Sysargs));
	sysArgs->number = SYS_TERMINATE;
	sysArgs->arg1 = (void*)rc;
	USLOSS_Syscall(sysArgs);
	/*Never gets here*/
	USLOSS_Console("Got Here\n");
	return rc;
}

void P2_Terminate(int status) {
	if (permissionCheck()) {
		return;
	}
	P1_Quit(status);
}

int P2_Wait(int *status){
        if (permissionCheck()) {
                return -1;
        }
	return P1_Join(status);
}

static int ClockDriver(void *arg) {
	P1_Semaphore running = (P1_Semaphore) arg;
	int result;
	int status;
	int rc = 0;

	/*
	 * Let the parent know we are running and enable interrupts.
	 */
	P1_V(running);
	while (1) {
		goto done;
		result = P1_WaitDevice(USLOSS_CLOCK_DEV, 0, &status);
		if (result != 0) {
			rc = 1;
			goto done;
		}
		/*
		 * Compute the current time and wake up any processes
		 * whose time has come.
		 */
	}
	done: return rc;
}

void kernelMode(void) {
	USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_MODE);
}

void userMode(void) {
	USLOSS_PsrSet(USLOSS_PsrGet() & ~(USLOSS_PSR_CURRENT_MODE));
}
