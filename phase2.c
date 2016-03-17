/*
 452 Phase 2 Part A
 Authors:
 Matt Seall
 Zachary Nixon
 */
#include <stdlib.h>
#include <phase1.h>
#include <phase2.h>
#include "usyscall.h"
#include <stdio.h>
#include <string.h>
#include <libuser.h>

#define US_IN_S 1000000

/* Structs */

typedef struct userArgStruct {
	int (*func)(void*);
	void *arg;
} userArgStruct;

typedef struct mailboxMessage{
	void *buf;
	int bufSize;
	struct mailboxMessage *next;
} message;

typedef struct clockDriverStruct{
	int inUse;
	P1_Semaphore sem;
	long startTime;
	long waitingTime;
} clockDriverStruct;

typedef struct mailbox{
	int id;
	int inUse;
	int maxSlots;
	int activeSlots;
	int maximumMessageSize;
	P1_Semaphore mutex;
	P1_Semaphore fulls;
	P1_Semaphore empties;
	message *queue;
} mailbox;

/*Prototypes*/
static int ClockDriver(void *arg);
static int launch(void *arg);

static void queueInsert(void *m,int size,message **head);
static message * queuePop(message **head);

void sysHandler(int type, void *arg);

static int permissionCheck(void);
static void interruptsOn(void);
static void userMode(void);

/*All mailboxes*/
mailbox mailboxes[P2_MAX_MBOX];

/*The list of processes waiting on the clock driver struct*/
clockDriverStruct clockList[P1_MAXPROC];

/*Guard mailbox list*/
P1_Semaphore mBoxSemaphore;

/*Guard clock list*/
P1_Semaphore clockListSemaphore;
int P2_Startup(void *arg) {
	USLOSS_IntVec[USLOSS_SYSCALL_INT] = sysHandler;
	/*Init all mailboxes*/
	int i;
	for(i = 0; i < P2_MAX_MBOX;i++){
		mailboxes[i].id = i;
		mailboxes[i].inUse = 0;
		mailboxes[i].queue = NULL;
		mailboxes[i].activeSlots = 0;
	}

	for(i = 0; i < P1_MAXPROC;i++){
		clockList[i].inUse = 0;
	}

	mBoxSemaphore = P1_SemCreate(1);
	clockListSemaphore = P1_SemCreate(1);

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
	USLOSS_Halt(0);
	return 0;
}

void sysHandler(int type,void *arg) {
	if(arg == NULL){
		USLOSS_Console("USLOSS_Sysargs is NULL!\n");
		return;
	}
	USLOSS_Sysargs *sysArgs = (USLOSS_Sysargs *) arg;
	int retVal = 0;
	int retVal2 = 0;
	interruptsOn();
	switch (sysArgs->number) {
	case SYS_TERMREAD: 
		break;
	case SYS_TERMWRITE:
		break;
	case SYS_SPAWN: //Part 1
		retVal = P2_Spawn(sysArgs->arg5, sysArgs->arg1, sysArgs->arg2,
				(int) sysArgs->arg3, (int) sysArgs->arg4);
		if (retVal == -3 || retVal == -2) {
			sysArgs->arg4 = (void *) -1;
		} else {
			sysArgs->arg1 = (void *) retVal;
			sysArgs->arg4 = (void *) 0;
		}
		break;
	case SYS_WAIT: //Part 1
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
	case SYS_TERMINATE: //Part 1
		P2_Terminate((int)sysArgs->arg1);
		break;
	case SYS_SLEEP: // Part 1
		retVal = P2_Sleep((int)sysArgs->arg1);
		sysArgs->arg4 = (void *) retVal;
		break;
	case SYS_DISKREAD:
		break;
	case SYS_DISKWRITE:
		break;
	case SYS_GETTIMEOFDAY: //Part 1
		sysArgs->arg1 = (void *)USLOSS_Clock();
		break;
	case SYS_CPUTIME: //Part 1
		sysArgs->arg1 = (void *)P1_ReadTime();
		break;
	case SYS_DUMPPROCESSES: //Part 1
		P1_DumpProcesses();
		break;
	case SYS_GETPID: //Part 1
		sysArgs->arg1 = (void *) P1_GetPID();
		break;
	case SYS_SEMCREATE: //Part 1
		retVal = (int)sysArgs->arg1;
		if(retVal < 0){
			sysArgs->arg4 = (void *)-1;
		}else{
			sysArgs->arg4 = (void *)0;
			sysArgs->arg1 = (void *) P1_SemCreate(retVal);
		}
		break;
	case SYS_SEMP: // Part 1
		retVal = P1_P((P1_Semaphore)sysArgs->arg1);
		if(retVal < 0){
			sysArgs->arg4 = (void *) -1;
		}
		else{
			sysArgs->arg4 = (void *) 0;
		}
		break;
	case SYS_SEMV: // Part 1
		retVal = P1_V((P1_Semaphore)sysArgs->arg1);
		if(retVal < 0){
			sysArgs->arg4 = (void *) -1;
		}
		else{
			sysArgs->arg4 = (void *) 0;
		}
		break;
	case SYS_SEMFREE: // Part 1
		retVal = P1_SemFree((P1_Semaphore)sysArgs->arg1);
		if(retVal < 0){
			sysArgs->arg4 = (void *) -1;
		}
		else{
			sysArgs->arg4 = (void *) 0;
		}
		break;
	case SYS_MBOXCREATE: //Part 1
		retVal = P2_MboxCreate((int)sysArgs->arg1,(int)sysArgs->arg2);
		if(retVal == -2){
			sysArgs->arg1 = (void *) -1;
			sysArgs->arg4 = (void *) -1;
		}else{
			sysArgs->arg1 = (void *) retVal;
			sysArgs->arg4 = (void *) 0;
		}
		break;
	case SYS_MBOXRELEASE: // Part 1
		retVal = P2_MboxRelease((int)sysArgs->arg1);
		if(retVal < 0){
			sysArgs->arg4 = (void *) -1;
		}else{
			sysArgs->arg4 = (void *) 0;
		}
		break;
	case SYS_MBOXSEND: // Part 1
		retVal = P2_MboxSend((int)sysArgs->arg1,sysArgs->arg2,(int *)sysArgs->arg3);
		if(retVal < 0){
			sysArgs->arg4 = (void *) -1;
		}else{
			sysArgs->arg4 = (void *) 0;
		}
		break;
	case SYS_MBOXRECEIVE: // Part 1
                retVal = P2_MboxReceive((int)sysArgs->arg1,sysArgs->arg2,(int *)sysArgs->arg3);
                if(retVal < 0){
                        sysArgs->arg4 = (void *) -1;
                }else{
                        sysArgs->arg4 = (void *) 0;
			sysArgs->arg2 = (void *) retVal;
                }
		break;
	case SYS_MBOXCONDSEND: // Part 1
                retVal = P2_MboxCondSend((int)sysArgs->arg1,sysArgs->arg2,(int *)sysArgs->arg3);
                if(retVal < 0){
                        sysArgs->arg4 = (void *) -1;
                }else if(retVal == 1){
			sysArgs->arg4 = (void *) 1;
		}
		else{
                        sysArgs->arg4 = (void *) 0;
                }
		break;
	case SYS_MBOXCONDRECEIVE: // Part 1
                retVal = P2_MboxCondReceive((int)sysArgs->arg1,sysArgs->arg2,(int *)sysArgs->arg3);
                if(retVal < 0){
                        sysArgs->arg4 = (void *) -1;
                }else if(retVal == 1){
                        sysArgs->arg4 = (void *) 1;
                }
                else{
                        sysArgs->arg4 = (void *) 0;
			sysArgs->arg2 = (void *) retVal;
                }
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
	Sys_Terminate(rc);
	/*Never gets here*/
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

int P2_Sleep(int seconds){
	if(permissionCheck() || seconds < 0){
		return -1;
	}
	P1_Semaphore s = P1_SemCreate(0);
	P1_P(clockListSemaphore);
	int i;
	for(i = 0; i < P1_MAXPROC;i++){
		if(clockList[i].inUse == 0){
			clockList[i].inUse = 1;
			clockList[i].sem = s;
			clockList[i].startTime = USLOSS_Clock();
			clockList[i].waitingTime = seconds * US_IN_S;
		}
	}
	P1_V(clockListSemaphore);
	P1_P(s);
	P1_SemFree(s);
	return 0;
}

int P2_MboxCreate(int slots, int size){
	if(permissionCheck()){
		return -1;
	}
	P1_P(mBoxSemaphore);
	if(slots <= 0 || size < 0){
		P1_V(mBoxSemaphore);
		return -2;
	}
	int i;
	for(i = 0; i < P2_MAX_MBOX;i++){
		if(mailboxes[i].inUse == 0){
			mailboxes[i].inUse = 1;
			mailboxes[i].maxSlots = slots;
			mailboxes[i].maximumMessageSize = size;
			mailboxes[i].mutex = P1_SemCreate(1);
			mailboxes[i].empties = P1_SemCreate(slots);
			mailboxes[i].fulls = P1_SemCreate(0);
			mailboxes[i].queue = NULL;
			mailboxes[i].activeSlots = 0;
			P1_V(mBoxSemaphore);
			return i;
		}
	}
	P1_V(mBoxSemaphore);
	return -1;
}

int P2_MboxRelease(int mbox){
	if(permissionCheck()){
		return -1;
	}
	P1_P(mBoxSemaphore);
	int i;
	for(i = 0; i < P2_MAX_MBOX;i++){
		if(mailboxes[i].inUse && mailboxes[i].id == mbox){
			if(P1_SemFree(mailboxes[i].mutex) != 0){
				USLOSS_Console("Processes waiting on mailbox. Halting.\n");
				USLOSS_Halt(1);
			}
			if(P1_SemFree(mailboxes[i].fulls) != 0){
				USLOSS_Console("Processes waiting on mailbox. Halting.\n");
				USLOSS_Halt(1);
			}
			if(P1_SemFree(mailboxes[i].empties) != 0){
				USLOSS_Console("Processes waiting on mailbox. Halting.\n");
				USLOSS_Halt(1);
			}

			while(mailboxes[i].queue != NULL){
				message *m = queuePop(&(mailboxes[i].queue));
				free(m->buf);
				free(m);
			}
			mailboxes[i].inUse = 0;
			P1_V(mBoxSemaphore);
			return 0;
		}
	}
	P1_V(mBoxSemaphore);
	return -1;
}

int P2_MboxSend(int mbox, void *msg, int *size){
	if(mbox < 0 || mbox >= P2_MAX_MBOX || mailboxes[mbox].inUse != 1 || mailboxes[mbox].maximumMessageSize < *size){
		return -1;
	}

	mailbox *cur = &(mailboxes[mbox]);
	P1_P(cur->empties);
	P1_P(cur->mutex);
	void *m = malloc(*size);
	memcpy(m,msg,*size);
	queueInsert(m,*size,&(cur->queue));
	cur->activeSlots++;
	P1_V(cur->mutex);
	P1_V(cur->fulls);
	return 0;
}

int P2_MboxCondSend(int mbox, void *msg, int *size){
	if(mbox < 0 || mbox >= P2_MAX_MBOX || mailboxes[mbox].inUse != 1 || mailboxes[mbox].maximumMessageSize < *size){
		return -1;
	}

	mailbox *cur = &(mailboxes[mbox]);
	P1_P(cur->mutex);
	if(cur->activeSlots < cur->maxSlots){
		P1_P(cur->empties); //Never blocks.
		void *m = malloc(*size);
		memcpy(m,msg,*size);
		queueInsert(m,*size,&(cur->queue));
		cur->activeSlots++;
		P1_V(cur->mutex);
		P1_V(cur->fulls);
		return 0;
	}
	P1_V(cur->mutex);
	return 1;
}

int P2_MboxReceive(int mbox, void *msg, int *size){
	if(mbox < 0 || mbox >= P2_MAX_MBOX || mailboxes[mbox].inUse != 1){
		return -1;
	}
	mailbox *cur = &(mailboxes[mbox]);
	P1_P(cur->fulls);
	P1_P(cur->mutex);

	message *m = queuePop(&(cur->queue));
	int retVal = 0;
	if(*size > m->bufSize){
		retVal = m->bufSize;
	}else{
		retVal = *size;
	}
	memcpy(msg,m->buf,retVal);
	free(m->buf);
	free(m);
	cur->activeSlots--;
	P1_V(cur->mutex);
	P1_V(cur->empties);
	return retVal;
}

/*
 * 0 >= bytes write
 * -1 = invalid args
 * -2 = no message
 */
int P2_MboxCondReceive(int mbox, void *msg, int *size){
	if(mbox < 0 || mbox >= P2_MAX_MBOX || mailboxes[mbox].inUse != 1){
		return -1;
	}
	mailbox *cur = &(mailboxes[mbox]);
	P1_P(cur->mutex);
	if(cur->activeSlots == 0){
		P1_P(cur->fulls); //Never blocks
		message *m = queuePop(&(cur->queue));
		int retVal = 0;
		if(*size > m->bufSize){
			retVal = m->bufSize;
		}else{
			retVal = *size;
		}
		memcpy(msg,m->buf,retVal);
		free(m->buf);
		free(m);
		cur->activeSlots--;
		P1_V(cur->mutex);
		P1_V(cur->empties);
		return retVal;
	}
	P1_V(cur->mutex);
	return -2;
}

static int ClockDriver(void *arg) {
	interruptsOn();
	P1_Semaphore running = (P1_Semaphore) arg;
	int result;
	int status;
	int rc = 0;
	int myPID = P1_GetPID();
	/*
	 * Let the parent know we are running and enable interrupts.
	 */
	P1_V(running);
	while (1) {
		if(P1_GetState(myPID) == 2){
			goto done;
		}	
		result = P1_WaitDevice(USLOSS_CLOCK_DEV, 0, &status);
		if (result != 0) {
			rc = 1;
			goto done;
		}
		/*
		 * Compute the current time and wake up any processes
		 * whose time has come.
		 */
		P1_P(clockListSemaphore);
		int i;
		long now = USLOSS_Clock();
		for(i = 0; i < P1_MAXPROC;i++){
			if(clockList[i].inUse == 1){
				if((now - clockList[i].startTime) >= clockList[i].waitingTime){
					P1_V(clockList[i].sem);
					clockList[i].inUse = 0;
				}
			}
		}

		P1_V(clockListSemaphore);
	}
	done: return rc;
}

void queueInsert(void *m,int size,message **head) {
	message *node = malloc(sizeof(message));
	node->buf = m;
	node->next = NULL;
	node->bufSize = size;

	if (*head == NULL) {
		*head = node;
		return;
	}

	message *temp = *head;
	while (temp->next != NULL) {
		temp = temp->next;
	}
	temp->next = node;
}

message * queuePop(message **head) {
	message *tmp = *head;
	*head = (*head)->next;
	return tmp;
}

/*
 0 == we are in kernel mode. continue.
 1 == we are not in kernel mode. error message.
 */
static int permissionCheck(void) {
	if ((USLOSS_PsrGet() & 0x1) < 1) {
		USLOSS_Console("Must be in Kernel mode to perform this request. Stopping requested operation\n");
		return 1;
	}
	return 0;
}

static void interruptsOn(void) {
	USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
}

static void userMode(void) {
	USLOSS_PsrSet(USLOSS_PsrGet() & ~(USLOSS_PSR_CURRENT_MODE));
}
