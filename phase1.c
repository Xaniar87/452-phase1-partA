/*
 452 Phase 1 Part B
 Authors:
 Matt Seall
 Zachary Nixon
 */

#include <stddef.h>
#include "usloss.h"
#include "phase1.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef enum States {
	UNUSED, RUNNING, READY, KILLED, QUIT, BLOCKED
} State;

/* -------------------------- Globals ------------------------------------- */

typedef struct PCB {
	char *name;
	USLOSS_Context context;
	int (*startFunc)(void *); /* Starting function */
	void *startArg; /* Arg to starting function */
	int parentPid;
	int priority;
	State state;
	int numChildren;
	int cpuTime;
	int startTime;
	int killedStatus;
	int pid;
	void* processStack;
} PCB;

typedef struct node {
	PCB *pcb;
	struct node *next;
} queueNode;

#define LOWEST_PRIORITY 6
#define HIGHEST_PRIORITY 0

/* the process table */
PCB procTable[P1_MAXPROC];

queueNode *readyQueue = NULL;
queueNode *blockedQueue = NULL;

/* current process ID */
int pid = -1;

/* number of processes */
int numProcs = 0;
USLOSS_Context dispatcher_context;

int startUpDone = 0;

static int sentinel(void *arg);
static void launch(void);
PCB * queuePriorityPop(int ready);
void queueInsert(PCB *pcb, int ready);

void syscallHandler(int type,void *arg){
        char string[50];
        sprintf(string,"System call %d not implemented\n",type);
        P1_Quit(1);
}

void clockIntHandler(int type, void *arg){
	printf("Clock int\n");
}

void countDownIntHandler(int type, void *arg){
	return;
}

void terminalIntHandler(int type, void *arg){
	return;
}

void diskIntHandler(int type, void *arg){
	return;
}

void MMUIntHandler(int type, void *arg){
	return;
}

/*
 0 == we are in kernel mode. continue.
 1 == we are not in kernel mode. error message.
 */
int permissionCheck(void) {
	if ((USLOSS_PsrGet() & 0x1) < 1) {
		USLOSS_Console(
				"Must be in Kernel mode to perform this request. Stopping requested operation\n");
		return 1;
	}
	return 0;
}

/* -------------------------- Functions ----------------------------------- */
/* ------------------------------------------------------------------------
 Name - dispatcher
 Purpose - runs the highest priority runnable process
 Parameters - none
 Returns - nothing
 Side Effects - runs a process
 ----------------------------------------------------------------------- */

void dispatcher(void) {
	while (1) {
		PCB *p = queuePriorityPop(1);
		pid = p->pid;
		if (p->state == READY) {
			p->state = RUNNING;
		}
		p->startTime = USLOSS_Clock();
		USLOSS_ContextSwitch(&dispatcher_context, &(p->context));
		int finTime = USLOSS_Clock();
		p->cpuTime = (p->cpuTime + (finTime - p->startTime));
	}
}

/* ------------------------------------------------------------------------
 Name - startup
 Purpose - Initializes semaphores, process lists and interrupt vector.
 Start up sentinel process and the P2_Startup process.
 Parameters - none, called by USLOSS
 Returns - nothing
 Side Effects - lots, starts the whole thing
 ----------------------------------------------------------------------- */
void startup() {

	/* initialize the process table here
	 loop through array and initialize base values for PCB*/
	int i;
	for (i = 0; i < P1_MAXPROC; i++) {
		procTable[i].state = UNUSED;
		procTable[i].cpuTime = 0;
		procTable[i].numChildren = 0;
		procTable[i].parentPid = -1;
		procTable[i].cpuTime = 0;
		procTable[i].killedStatus = 0;
		procTable[i].pid = i;
	}

	/* Initialize interrupt handlers */
	USLOSS_IntVec[USLOSS_SYSCALL_INT] = syscallHandler;
	USLOSS_IntVec[USLOSS_CLOCK_INT] = clockIntHandler;
	USLOSS_IntVec[USLOSS_ALARM_INT] = countDownIntHandler;
	USLOSS_IntVec[USLOSS_TERM_INT] = terminalIntHandler;
	USLOSS_IntVec[USLOSS_DISK_INT] = diskIntHandler;
	USLOSS_IntVec[USLOSS_MMU_INT] = MMUIntHandler;


	P1_Fork("sentinel", sentinel, NULL, USLOSS_MIN_STACK, 6);

	P1_Fork("P2_Startup", P2_Startup, NULL, 4 * USLOSS_MIN_STACK, 1);

	/*Start the dispatcher here*/
	void *stack = malloc(USLOSS_MIN_STACK);
	USLOSS_ContextInit(&dispatcher_context, USLOSS_PsrGet(), stack,
			USLOSS_MIN_STACK, &dispatcher);
	startUpDone = 1;
	USLOSS_ContextSwitch(NULL, &dispatcher_context);

	/* Should never get here (sentinel will call USLOSS_Halt) */

	return;
} /* End of startup */

/* ------------------------------------------------------------------------
 Name - finish
 Purpose - Required by USLOSS
 Parameters - none
 Returns - nothing
 Side Effects - none
 ----------------------------------------------------------------------- */
void finish() {
	USLOSS_Console("Goodbye.\n");
} /* End of finish */

/* ------------------------------------------------------------------------
 Name - P1_Fork
 Purpose - Gets a new process from the process table and initializes
 information of the process.  Updates information in the
 parent process to reflect this child process creation.
 Parameters - the process procedure address, the size of the stack and
 the priority to be assigned to the child process.
 Returns - the process id of the created child or an error code.
 Side Effects - ReadyList is changed, procTable is changed, Current
 process information changed
 ------------------------------------------------------------------------ */
int P1_Fork(char *name, int (*f)(void *), void *arg, int stacksize,
		int priority) {
	if (permissionCheck()) {
		return -1;
	}
	if (stacksize < USLOSS_MIN_STACK) {
		return -2;
	}
	if (priority < 0 || priority > 5) {
		if (priority == 6 && strcmp(name, "sentinel") == 0) {
			;
		} else {
			return -3;
		}
	}
	int newPid = -1;
	void *stack = NULL;
	/* newPid = pid of empty PCB here */
	int i = 0;
	for (i = 0; i < P1_MAXPROC; i++) {
		if (procTable[i].state == UNUSED) {
			newPid = i;
			procTable[i].state = READY;
			procTable[i].parentPid = pid;
			procTable[i].priority = priority;
			// increment the current process's children count for it
			procTable[pid].numChildren = procTable[pid].numChildren + 1;
			if (procTable[pid].processStack != NULL) {
				free(procTable[i].processStack);
				procTable[i].processStack = NULL;
			}
			break;
		}
	}

	if (newPid == -1) {
		return -1;
	}
	procTable[newPid].startFunc = f;
	procTable[newPid].startArg = arg;
	procTable[newPid].name = strdup(name);
	stack = malloc(stacksize);
	procTable[newPid].processStack = stack;
	USLOSS_ContextInit(&(procTable[newPid].context), USLOSS_PsrGet(), stack,
			stacksize, launch);
	numProcs++;

	queueInsert(&(procTable[newPid]), 1);
	if (startUpDone && (priority < procTable[pid].priority)) {
		if (procTable[pid].state == RUNNING) {
			procTable[pid].state = READY;
		}
		queueInsert(&(procTable[pid]), 1);
		// switching contexts so get new start time from cpu
		USLOSS_ContextSwitch(&(procTable[pid].context), &dispatcher_context);
	}
	return newPid;
} /* End of fork */

/* ------------------------------------------------------------------------
 Name - launch
 Purpose - Dummy function to enable interrupts and launch a given process
 upon startup.
 Parameters - none
 Returns - nothing
 Side Effects - enable interrupts
 ------------------------------------------------------------------------ */
void launch(void) {
	int rc;
	USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
	rc = procTable[pid].startFunc(procTable[pid].startArg);
	/* quit if we ever come back */
	P1_Quit(rc);
} /* End of launch */

/* ------------------------------------------------------------------------
 Name - P1_Quit
 Purpose - Causes the process to quit and wait for its parent to call P1_Join.
 Parameters - quit status
 Returns - nothing
 Side Effects - the currently running process quits
 ------------------------------------------------------------------------ */
void P1_Quit(int status) {
	if (permissionCheck()) {
		return;
	}
	procTable[pid].state = UNUSED;
	procTable[pid].numChildren = 0;
	procTable[pid].startTime = 0;
	procTable[pid].cpuTime = 0;
	procTable[pid].parentPid = 0;
	int i;
	// let other processes know that they are orphans by setting parentPID = -1
	for (i = 0; i < P1_MAXPROC; i++) {
		if (procTable[i].parentPid == pid) {
			procTable[i].parentPid = -1;
		}
	}
	numProcs--;
	USLOSS_ContextSwitch(NULL, &dispatcher_context);
}

/* ------------------------------------------------------------------------
 Name - sentinel
 Purpose - The purpose of the sentinel routine is two-fold.  One
 responsibility is to keep the system going when all other
 processes are blocked.  The other is to detect and report
 simple deadlock states.
 Parameters - none
 Returns - nothing
 Side Effects -  if system is in deadlock, print appropriate error
 and halt.
 ----------------------------------------------------------------------- */
int sentinel(void *notused) {
	while (numProcs > 1) {
		/* Check for deadlock here */
		USLOSS_WaitInt();
	}
	USLOSS_Halt(0);
	/* Never gets here. */
	return 0;
} /* End of sentinel */

int P1_GetPID(void) {
	if (permissionCheck()) {
		return -1;
	}
	return pid;
}

int P1_GetState(int pid) {
	if (permissionCheck()) {
		return -1;
	}
	if (pid >= 0 && pid < P1_MAXPROC) {
		if (procTable[pid].state == RUNNING) {
			return 0;
		} else if (procTable[pid].state == READY) {
			return 1;
		} else if (procTable[pid].state == KILLED) {
			return 2;
		} else if (procTable[pid].state == BLOCKED) {
			return 4;
		} else {
			return 3;
		}
	}
	/* Invalid pid */
	return -1;
}

/*
 print process info to console for debuggin purposes
 for each PCB in process table, print its PID, parents PID,
 priority,process state,# of chilren,CPU time consumed,and name
 */

void P1_DumpProcesses(void) {
	if (permissionCheck()) {
		return;
	}
	int i;
	char string[5000] = { '\0' };
	int bytes = 0;
	bytes += sprintf(string, "%-20s%-20s%-20s%-20s%-20s%-20s%-20s", "Name",
			"Pid", "Parent", "Priority", "State", "# Children", "time(uS)");
	printf("%s\n", string);
	for (i = 0; i < P1_MAXPROC; i++) {
		if (procTable[i].state != UNUSED) {
			bytes = 0;
			string[0] = 0;
			bytes += sprintf(string, "%-20s", procTable[i].name);
			bytes += sprintf(string + bytes, "%-20d", i);
			bytes += sprintf(string + bytes, "%-20d", procTable[i].parentPid);
			bytes += sprintf(string + bytes, "%-20d", procTable[i].priority);
			if (procTable[i].state == RUNNING) {
				bytes += sprintf(string + bytes, "%-20s", "RUNNING");
			}
			if (procTable[i].state == READY) {
				bytes += sprintf(string + bytes, "%-20s", "READY");
			}
			if (procTable[i].state == KILLED) {
				bytes += sprintf(string + bytes, "%-20s", "KILLED");
			}
			if (procTable[i].state == QUIT) {
				bytes += sprintf(string + bytes, "%-20s", "QUIT");
			}
			if (procTable[i].state == BLOCKED) {
				bytes += sprintf(string + bytes, "%-20s", "BLOCKED");
			}
			bytes += sprintf(string + bytes, "%-20d", procTable[i].numChildren);
			if (i == pid) {
				bytes += sprintf(string + bytes, "%-20d\n", P1_ReadTime());
			} else {
				bytes += sprintf(string + bytes, "%-20d\n",
						procTable[i].cpuTime);
			}
			USLOSS_Console(string);
		}
	}
}

int P1_Kill(int p) {
	if (permissionCheck()) {
		return -1;
	}
	if (p == pid) {
		return -2;
	}
	printf("%d\n", procTable[p].state);
	if (p >= 0 && p < P1_MAXPROC) {
		if (procTable[p].state == READY || procTable[p].state == BLOCKED) {
			printf("HEre\n");
			procTable[p].state = KILLED;
			procTable[p].killedStatus = 0;
		}
		return 0;
	}
	return -1;
}

int P1_ReadTime(void) {
	if (permissionCheck()) {
		return -1;
	}
	int finTime = USLOSS_Clock();
	return (procTable[pid].cpuTime + (finTime - procTable[pid].startTime));
}

void queueInsert(PCB *pcb, int ready) {
	queueNode *node = malloc(sizeof(queueNode));
	node->pcb = pcb;
	node->next = NULL;
	queueNode *temp;
	if (ready) {
		if (readyQueue == NULL) {
			readyQueue = node;
			return;
		} else if (readyQueue->pcb->priority > pcb->priority) {
			node->next = readyQueue;
			readyQueue = node;
			return;
		}
		temp = readyQueue;
	} else {
		if (blockedQueue == NULL) {
			blockedQueue = node;
			return;
		} else if (blockedQueue->pcb->priority > pcb->priority) {
			node->next = blockedQueue;
			blockedQueue = node;
			return;
		}
		temp = blockedQueue;
	}

	while (temp->next != NULL && pcb->priority >= temp->next->pcb->priority) {
		temp = temp->next;
	}

	if (temp->next == NULL) {
		temp->next = node;
	} else {
		node->next = temp->next;
		temp->next = node;
	}
}

/*Debugging function*/
void printList() {
	queueNode *tmp = readyQueue;
	while (tmp) {
		printf("%d\n", tmp->pcb->priority);
		tmp = tmp->next;
	}
}

PCB * queuePriorityPop(int ready) {
	PCB *pcb;
	if (ready) {
		pcb = readyQueue->pcb;
		queueNode *tmp = readyQueue;
		readyQueue = readyQueue->next;
		free(tmp);
	} else {
		pcb = blockedQueue->pcb;
		queueNode *tmp = blockedQueue;
		blockedQueue = blockedQueue->next;
		free(tmp);
	}
	return pcb;
}
