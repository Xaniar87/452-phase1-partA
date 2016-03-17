/*
 452 Phase 1 Part B
 Authors:
 Matt Seall
 Zachary Nixon
 */

#include <stddef.h>
#include "phase1.h"
#include "phase2.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef enum States {
        UNUSED, RUNNING, READY, KILLED, QUIT
} State;

struct semaphore;
struct node;

typedef struct PCB {
        char *name;
        USLOSS_Context context;
        int (*startFunc)(void *); /* Starting function */
        void *startArg; /* Arg to starting function */
        int parentPid;
        int priority;
        State state; /* Process state */
        int numChildren; /* Number of child processes */
        int cpuTime; /*Total time used by process*/
        int startTime; /*Time when the Process started running*/
        int killedStatus; /* status given to process by kill() */
        int pid;
        struct semaphore *sem;  // id for the semaphore children operate on
        struct semaphore *blockedSem; /*Semaphore that this process is blocked on*/
        struct node *deadChildren; /*List of children that have quit without being joined.*/
        int blocked; /*Is this process blocked?*/
        void* processStack; /*Process stack for context*/
} PCB;

typedef struct node {
        PCB *pcb;
        struct node *next;
} queueNode;

typedef struct semaphore {
        unsigned int count;
        int id;
        int inUse;
        queueNode *queue;
} semaphore;

/* -------------------------- Globals ------------------------------------- */

int clockSumTicks = 0;
int clockIntTicks = 0;

#define LOWEST_PRIORITY 6
#define HIGHEST_PRIORITY 0

/* the process table */
static PCB procTable[P1_MAXPROC];

/*Array of semaphores*/
semaphore semaphoreTable[P1_MAXSEM];

/*Device semaphores*/
semaphore *clockDevSemaphore;
int clockDevStatus = 0;
semaphore *alarmDevSemaphore;
int alarmDevStatus = 0;
semaphore *terminalDevSemaphores[USLOSS_TERM_UNITS] = { NULL };
int terminalDevStatus[USLOSS_TERM_UNITS] = { 0 };
semaphore *diskDevSemaphores[USLOSS_DISK_UNITS] = { NULL };
int diskDevStatus[USLOSS_DISK_UNITS] = { 0 };

queueNode *readyQueue = NULL;

/* current process ID */
int pid = -1;

/* number of processes */
int numProcs = 0;
USLOSS_Context dispatcher_context;

/* Flag for Fork - Determines if we should invoke dispatcher*/
int startUpDone = 0;

/* OS control functions */
static int sentinel(void *arg);
static void launch(void);
static void interruptsOff(void);
static void interruptsOn(void);

/* Interrupt Handlers */
void clockIntHandler(int type, void *arg);
void countDownIntHandler(int type, void *arg);
void terminalIntHandler(int type, void *arg);
void diskIntHandler(int type, void *arg);
void MMUIntHandler(int type, void *arg);

/*Queue functions*/
static PCB * queuePop(queueNode **head);
static void queuePriorityInsert(PCB *pcb, queueNode **head);
static void queueInsert(PCB *pcb, queueNode **head);

/*Misc*/
static int permissionCheck(void);
int checkInvalidSemaphore(P1_Semaphore sem);
int checkDeviceSemaphore(P1_Semaphore sem);
void prepareDispatcherSwap(int readyInsert);
void printList(queueNode *head);

/* ------------------------------------------------------------------------
 Name - dispatcher
 Purpose - runs the highest priority runnable process
 Parameters - none
 Returns - nothing
 Side Effects - runs a process
 ----------------------------------------------------------------------- */

void dispatcher(void) {
	while (1) {
		interruptsOff();
		PCB *p = queuePop(&readyQueue);
		pid = p->pid;
		if (p->state == READY) {
			p->state = RUNNING;
		}
		clockIntTicks = 0;
		p->startTime = USLOSS_Clock();
		interruptsOn();
		USLOSS_ContextSwitch(&dispatcher_context, &(p->context));
		p->cpuTime = P1_ReadTime();
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

	int i;

	/*Initialize semaphores*/
	for (i = 0; i < P1_MAXSEM; i++) {
		semaphoreTable[i].id = i;
		semaphoreTable[i].inUse = 0;
		semaphoreTable[i].count = 0;
		semaphoreTable[i].queue = NULL;
	}

	/*Initialize process table*/
	for (i = 0; i < P1_MAXPROC; i++) {
		procTable[i].state = UNUSED;
		procTable[i].cpuTime = 0;
		procTable[i].numChildren = 0;
		procTable[i].parentPid = -1;
		procTable[i].cpuTime = 0;
		procTable[i].killedStatus = 0;
		procTable[i].pid = i;
		procTable[i].deadChildren = NULL;
		procTable[i].sem = NULL;
	}

	/*Initialize device semaphores*/

	clockDevSemaphore = P1_SemCreate(0);
	alarmDevSemaphore = P1_SemCreate(0);
	for(i = 0; i < USLOSS_TERM_UNITS;i++){
		terminalDevSemaphores[i] = P1_SemCreate(0);
	}
	for(i = 0; i < USLOSS_DISK_UNITS;i++){
		diskDevSemaphores[i] = P1_SemCreate(0);
	}

	/* Initialize interrupt handlers */
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
	interruptsOff();
	if (stacksize < USLOSS_MIN_STACK) {
		interruptsOn();
		return -2;
	}
	if (priority < 0 || priority > 5) {
		if (priority == 6 && strcmp(name, "sentinel") == 0) {
			;
		} else {
			interruptsOn();
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
			procTable[i].cpuTime = 0;
			procTable[i].numChildren = 0;
			procTable[i].startTime = 0;
			procTable[i].killedStatus = 0;
			procTable[i].blocked = 0;
			if (procTable[i].processStack != NULL) {
				free(procTable[i].processStack);
				procTable[i].processStack = NULL;
			}
			if(procTable[i].sem != NULL){
				P1_SemFree(procTable[i].sem);
			}
			procTable[i].sem = (semaphore *) P1_SemCreate(0);
			break;
		}
	}

	if (newPid == -1) {
		interruptsOn();
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
	procTable[pid].numChildren++;
	queuePriorityInsert(&(procTable[newPid]),&readyQueue);
	if (startUpDone && (priority < procTable[pid].priority)) {
		prepareDispatcherSwap(1);
	}
	interruptsOn();
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
	interruptsOn();
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
	interruptsOff();
	numProcs--;
	procTable[pid].state = QUIT;
	procTable[pid].numChildren = 0;
	procTable[pid].startTime = 0;
	procTable[pid].cpuTime = 0;
	procTable[pid].killedStatus = status;
	if(procTable[pid].parentPid >= 0){
		procTable[procTable[pid].parentPid].numChildren--;
	}
	int i;
	// let other processes know that they are orphans by setting parentPID = -1
	for (i = 0; i < P1_MAXPROC; i++) {
		if (procTable[i].parentPid == pid) {
			procTable[i].parentPid = -1;
			//Allow processes that have quit but not joined() yet to be overwritten
			if(procTable[i].state == QUIT){
				procTable[i].state = UNUSED;
				procTable[i].killedStatus = -1;
				procTable[i].parentPid = -1;
				free(procTable[i].name);
			}
		}
	}

	/*Orphan - Just allow for overwrite*/
	if(procTable[pid].parentPid == -1){
		procTable[pid].state = UNUSED;
		procTable[pid].killedStatus = -1;
		procTable[pid].parentPid = -1;
		free(procTable[pid].name);
	}else{
		/*Need to let parent know that a child has quit*/
		int pPid = procTable[pid].parentPid;
		queueInsert(&procTable[pid],&(procTable[pPid].deadChildren));
        	/*Clear dead queue for next process using this PCB.*/
        	while(procTable[pid].deadChildren != NULL){
                	queuePop(&(procTable[pid].deadChildren));
        	}
		procTable[pid].parentPid = -1;
		P1_V(procTable[pPid].sem);
	}
	interruptsOn();
	USLOSS_ContextSwitch(NULL, &dispatcher_context);
}

int P1_Join(int *status) {
	if (permissionCheck()) {
		return -2;
	}
	interruptsOff();
	if(procTable[pid].numChildren <= 0){
		interruptsOn();
		return -1;
	}

	P1_P(procTable[pid].sem);
	
	PCB *dead = queuePop(&(procTable[pid].deadChildren));
	*status = dead->killedStatus;
	int deadPid = dead->pid;
	procTable[deadPid].state = UNUSED;
	procTable[deadPid].killedStatus = -1;
	free(procTable[deadPid].name);
	
	interruptsOn();
	return deadPid;
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
	int i;
	int deadLock = 1;
	while (numProcs > 1) {
		/* Check for deadlock here */
		for(i = 0; i < P1_MAXPROC;i++){
			if(procTable[i].blocked && checkDeviceSemaphore(procTable[i].blockedSem)){
				deadLock = 0;
				break;
			}
		}
		if(deadLock){
			P1_DumpProcesses();
			USLOSS_Console("Dead lock detected. Stopping.\n");
			USLOSS_Halt(1);
		}
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
		if(procTable[pid].blocked){
			return 4;
		}
		else if (procTable[pid].state == RUNNING) {
			return 0;
		} else if (procTable[pid].state == READY) {
			return 1;
		} else if (procTable[pid].state == KILLED) {
			return 2;
		} else {
			return 3;
		}
	}
	/* Invalid pid */
	return -1;
}

/*
 print process info to console for debugging purposes
 for each PCB in process table, print its PID, parents PID,
 priority,process state,# of children,CPU time consumed,and name
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
		if (procTable[i].state != UNUSED && procTable[i].state != QUIT) {
			bytes = 0;
			string[0] = 0;
			bytes += sprintf(string, "%-20s", procTable[i].name);
			bytes += sprintf(string + bytes, "%-20d", i);
			bytes += sprintf(string + bytes, "%-20d", procTable[i].parentPid);
			bytes += sprintf(string + bytes, "%-20d", procTable[i].priority);
			if (procTable[i].blocked) {
				bytes += sprintf(string + bytes, "%-20s", "BLOCKED");
			} else if (procTable[i].state == RUNNING) {
				bytes += sprintf(string + bytes, "%-20s", "RUNNING");
			} else if (procTable[i].state == READY) {
				bytes += sprintf(string + bytes, "%-20s", "READY");
			} else if (procTable[i].state == KILLED) {
				bytes += sprintf(string + bytes, "%-20s", "KILLED");
			} else if (procTable[i].state == QUIT) {
				bytes += sprintf(string + bytes, "%-20s", "QUIT");
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
	interruptsOff();
	if (p >= 0 && p < P1_MAXPROC) {
		if (procTable[p].state == READY || procTable[p].blocked) {
			procTable[p].state = KILLED;
			procTable[p].killedStatus = 0;
		}
		interruptsOn();
		return 0;
	}
	interruptsOn();
	return -1;
}

int P1_ReadTime(void) {
	if (permissionCheck()) {
		return -1;
	}
	int finTime = USLOSS_Clock();
	return (procTable[pid].cpuTime + (finTime - procTable[pid].startTime));
}

int P1_WaitDevice(int type, int unit, int *status) {
	if (permissionCheck()) {
		return -4;
	}

	interruptsOff();

	/*Cannot wait on a device if this process is killed.*/
	if(P1_GetState(pid) == 2){
		interruptsOn();
		return -3;
	}

	if(unit < 0){
		interruptsOn();
		return -1;
	}

	if(type == USLOSS_CLOCK_DEV){
		if(unit == 0){
			P1_P(clockDevSemaphore);
			*status = clockDevStatus;
		}else{
			interruptsOn();
			return -1;
		}
	}else if(type == USLOSS_ALARM_DEV){
		if(unit == 0){
			P1_P(alarmDevSemaphore);
			*status = alarmDevStatus;
		}else{
			interruptsOn();
			return -1;
		}
	}else if(type == USLOSS_DISK_DEV){
		if(unit >= 0 && unit < USLOSS_DISK_UNITS){
			P1_P(diskDevSemaphores[unit]);
			*status = diskDevStatus[unit];
		}else{
			interruptsOn();
			return -1;
		}
	}else if(type == USLOSS_TERM_DEV){
		if(unit >= 0 && unit < USLOSS_TERM_UNITS){
			P1_P(terminalDevSemaphores[unit]);
			*status = terminalDevStatus[unit];
		}else{
			interruptsOn();
			return -1;
		}
	}else{
		interruptsOn();
		return -2;
	}
	interruptsOn();
	return 0;
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

static void interruptsOff(void) {
	/*Thanks for the drill on this.*/
	USLOSS_PsrSet(USLOSS_PsrGet() & ~(USLOSS_PSR_CURRENT_INT));
}

void prepareDispatcherSwap(int insertReady) {
	if (procTable[pid].state == RUNNING) {
		procTable[pid].state = READY;
	}
	clockIntTicks = 0;
	if (insertReady && procTable[pid].state != QUIT) {
		queuePriorityInsert(&(procTable[pid]), &readyQueue);
	}
	interruptsOn();
	USLOSS_ContextSwitch(&(procTable[pid].context), &dispatcher_context);
}

/*Check if sem is a device semaphore
 * returns 0 for not a device semaphore
 * return 1 if is a device semaphore
 * */
int checkDeviceSemaphore(P1_Semaphore sem){
	if(sem == clockDevSemaphore){
		return 1;
	}
	if(sem == alarmDevSemaphore){
		return 1;
	}
	int i;
	for(i = 0; i < USLOSS_TERM_UNITS;i++){
		if(terminalDevSemaphores[i] == sem){
			return 1;
		}
	}

	for(i = 0; i < USLOSS_DISK_UNITS;i++){
		if(diskDevSemaphores[i] == sem){
			return 1;
		}
	}
	return 0;
}

/* Insert into a normal queue*/
void queueInsert(PCB *pcb, queueNode **head) {
	interruptsOff();
	queueNode *node = malloc(sizeof(queueNode));
	node->pcb = pcb;
	node->next = NULL;

	if (*head == NULL) {
		*head = node;
		interruptsOn();
		return;
	}

	queueNode *temp = *head;
	while (temp->next != NULL) {
		temp = temp->next;
	}
	temp->next = node;
	interruptsOn();
}

/*Insert into a priority queue*/
void queuePriorityInsert(PCB *pcb, queueNode **head) {
	interruptsOff();
	if(pcb->state == QUIT){
		interruptsOn();
		//return;
	}
	queueNode *new = malloc(sizeof(queueNode));
	new->next = NULL;
	new->pcb = pcb;
	if(*head == NULL){
		*head = new;
		interruptsOn();
		return;
	}

	if((*head)->pcb->priority > pcb->priority){
		new->next = *head;
		*head = new;
		interruptsOn();
		return;
	}
	
	queueNode *tmp = (*head)->next;
	queueNode *prev = *head;
	while(tmp != NULL && tmp->pcb->priority <= pcb->priority){
		prev = tmp;
		tmp = tmp->next;
	}

	if(tmp == NULL){
        	if(prev->pcb->priority > pcb->priority){
                	new->pcb = prev->pcb;
         	       	prev->pcb = pcb;
	        }
		prev->next = new;
	}else{
		new->next = prev->next;
		prev->next = new;
	}
	interruptsOn();
}
/*Debugging function*/
void printList(queueNode *head) {
	queueNode *tmp = head;
	while (tmp != NULL) {
		printf("%s %d\n", tmp->pcb->name,tmp->pcb->priority);
		tmp = tmp->next;
	}
}

PCB * queuePop(queueNode **head) {
	interruptsOff();
	PCB *pcb;
	queueNode *tmp = *head;
	*head = (*head)->next;
	pcb = tmp->pcb;
	free(tmp);
	interruptsOn();
	return pcb;
}

P1_Semaphore P1_SemCreate(unsigned int value) {
	if (permissionCheck()) {
		return NULL;
	}
	interruptsOff();
	int i;
	for (i = 0; i < P1_MAXSEM; i++) {
		if (semaphoreTable[i].inUse == 0) {
			semaphoreTable[i].count = value;
			semaphoreTable[i].queue = NULL;
			semaphoreTable[i].inUse = 1;
			interruptsOn();
			return &(semaphoreTable[i]);
		}
		
	}
	interruptsOn();
	/*Should never get here*/
	return NULL;
}

int P1_SemFree(P1_Semaphore sem) {
	if (permissionCheck()) {
		return -2;
	}
	interruptsOff();
	if (checkInvalidSemaphore(sem)) {
		return -1;
	}
	semaphore *s = (semaphore *) sem;
	if (s->inUse == 0) {
		interruptsOn();
		return -1;
	}

	if (s->queue != NULL) {
		USLOSS_Console(
				"P1_SemFree called on semaphore with blocked processes\n");
		USLOSS_Halt(1);
	}
	s->inUse = 0;
	s->count = 0;
	interruptsOn();
	return 0;
}

int P1_P(P1_Semaphore sem) {
	if (permissionCheck()) {
		return -2;
	}

	if (checkInvalidSemaphore(sem)) {
		return -1;
	}

	semaphore *s = (semaphore *) sem;
	if (s->inUse == 0) {
		return -1;
	}

	while (1) {
		interruptsOff();
		if (procTable[pid].state == KILLED) {
			interruptsOn();
			return -2;
		}
		if (s->count > 0) {
			s->count--;
			break;
		}

		queueInsert(&procTable[pid], &(s->queue));
		procTable[pid].blocked = 1;
		procTable[pid].blockedSem = sem;
		interruptsOn();
		prepareDispatcherSwap(0);
	}
	procTable[pid].blocked = 0;
	procTable[pid].blockedSem = NULL;
	interruptsOn();
	return 0;
}

int P1_V(P1_Semaphore sem) {
	if (permissionCheck()) {
		return -2;
	}
	if (checkInvalidSemaphore(sem)) {
		return -1;
	}

	semaphore *s = (semaphore *) sem;
	if (s->inUse == 0) {
		return -1;
	}

	interruptsOff();

	s->count++;
	if (s->queue != NULL) {
		PCB *p = queuePop(&(s->queue));
		queuePriorityInsert(p, &readyQueue);
		interruptsOn();
		prepareDispatcherSwap(1);
	} else {
		interruptsOn();
	}
	return 0;
}

/*Checks address of sem against ALL available user semaphores. Returns 1 if invalid semaphore. Returns 0 if valid.*/
int checkInvalidSemaphore(P1_Semaphore sem) {
	if(sem == NULL){
		return 1;
	}
	int i;
	for (i = 0; i < P1_MAXSEM; i++) {
		if (&(semaphoreTable[i]) == sem) {
			return 0;
		}
	}
	return 1;
}

void clockIntHandler(int type, void *arg) {
	clockSumTicks++;
	clockIntTicks++;
	if (clockSumTicks == 5) {
		USLOSS_DeviceInput(USLOSS_CLOCK_DEV,0,&clockDevStatus);
		P1_V(clockDevSemaphore);
		clockSumTicks = 0;
	}
	if (clockIntTicks == 4) {
		clockIntTicks = 0;
		prepareDispatcherSwap(1);
	}
}

void countDownIntHandler(int type, void *arg) {
	USLOSS_DeviceInput(USLOSS_ALARM_DEV,0,&alarmDevStatus);
	P1_V(alarmDevSemaphore);
}
void terminalIntHandler(int type, void *arg) {
	int unit = (int) arg;
	USLOSS_DeviceInput(USLOSS_TERM_DEV,unit,&(terminalDevStatus[unit]));
	P1_V(terminalDevSemaphores[unit]);
}

void diskIntHandler(int type, void *arg) {
	int unit = (int) arg;
	USLOSS_DeviceInput(USLOSS_DISK_DEV,unit,&(diskDevStatus[unit]));
	P1_V(diskDevSemaphores[unit]);
}

void MMUIntHandler(int type, void *arg) {
	return;
}
