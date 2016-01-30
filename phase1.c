/* ------------------------------------------------------------------------
   skeleton.c
 
   Skeleton file for Phase 1. These routines are very incomplete and are
   intended to give you a starting point. Feel free to use this or not.
 
 
   ------------------------------------------------------------------------ */
 
#include <stddef.h>
#include "usloss.h"
#include "phase1.h"
#include <stdlib.h>
#include <stdio.h>
 
/* -------------------------- Globals ------------------------------------- */
 
typedef struct PCB {
    char *name;
    USLOSS_Context      context;
    int                 (*startFunc)(void *);   /* Starting function */
    void                 *startArg;             /* Arg to starting function */
    int                  used;
    int                  kidPid;
    int                  priority;
} PCB;
 
#define LOWEST_PRIORITY 6
#define HIGHEST_PRIORITY 0 

/* the process table */
PCB procTable[P1_MAXPROC];
  
/* current process ID */
int pid = -1;
 
/* number of processes */ 
int numProcs = 0;
USLOSS_Context dispatcher_context;
 
static int sentinel(void *arg);
static void launch(void);
 
/* -------------------------- Functions ----------------------------------- */
/* ------------------------------------------------------------------------
   Name - dispatcher
   Purpose - runs the highest priority runnable process
   Parameters - none
   Returns - nothing
   Side Effects - runs a process
   ----------------------------------------------------------------------- */
 
void dispatcher(void)
{
  /*
   * Run the highest priority runnable process. There is guaranteed to be one
   * because the sentinel is always runnable.
   */
   while(1){
	int curr_priority;
	int j;
	for(curr_priority = HIGHEST_PRIORITY; curr_priority < LOWEST_PRIORITY + 1;curr_priority++){
   		 for(j = 0; j < P1_MAXPROC;j++){
        		if(procTable[j].used == 1 && procTable[j].priority == curr_priority){
				pid = j;
				USLOSS_ContextSwitch(&dispatcher_context,&procTable[j].context);
                		goto done;
        		}
    		}
	}
	done:;
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
void startup()
{
 
  /* initialize the process table here */
 
  /* Initialize the Ready list, Blocked list, etc. here */
 
  /* Initialize the interrupt vector here */
 
  /* Initialize the semaphores here */
 
  /* startup a sentinel process */
  /* HINT: you don't want any forked processes to run until startup is finished.
   * You'll need to do something in your dispatcher to prevent that from happening.
   * Otherwise your sentinel will start running right now and it will call P1_Halt. 
   */
  P1_Fork("sentinel", sentinel, NULL, USLOSS_MIN_STACK, 6);
 
  /* start the P2_Startup process */
  P1_Fork("P2_Startup", P2_Startup, NULL, 4 * USLOSS_MIN_STACK, 1);

  void *stack = malloc(USLOSS_MIN_STACK);
  USLOSS_ContextInit(&dispatcher_context, USLOSS_PsrGet(), stack,USLOSS_MIN_STACK, &dispatcher);
  USLOSS_ContextSwitch(NULL,&dispatcher_context);
 
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
void finish()
{
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
int P1_Fork(char *name, int (*f)(void *), void *arg, int stacksize, int priority)
{
    if(stacksize < USLOSS_MIN_STACK){
        return -2;
    }
    if(priority < 0 || priority > 6){
	return -3;
    }
    int newPid = -1;
    void *stack = NULL;
    /* newPid = pid of empty PCB here */
    int i = 0;
    for(i = 0; i < P1_MAXPROC;i++){
	if(procTable[i].used == 0){
		newPid = i;
		procTable[i].used = 1;
		procTable[i].kidPid = pid;
		procTable[i].priority = priority;
		break;
	}
    }

    if(newPid == -1){
	return -1;
    }
    procTable[newPid].startFunc = f;
    procTable[newPid].startArg = arg;
    procTable[newPid].name = name;
    stack = malloc(stacksize);
    USLOSS_ContextInit(&(procTable[newPid].context), USLOSS_PsrGet(), stack, 
        stacksize, launch);
    numProcs++;
    /* USLOSS_ContextSwitch(&(procTable[pid].context),&dispatcher_context); */
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
void launch(void)
{
  int  rc;
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
  // Do something here.
  procTable[pid].used = 0;
  numProcs--;
  USLOSS_ContextSwitch(NULL,&dispatcher_context);
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
int sentinel (void *notused)
{
    while (numProcs > 1)
    {
        /* Check for deadlock here */
        USLOSS_WaitInt();
    }
    USLOSS_Halt(0);
    /* Never gets here. */
    return 0;
} /* End of sentinel */

void P1_DumpProcesses(void){
	int i;
	for(i = 0; i < P1_MAXPROC;i++){
		if(procTable[i].used == 1){
			printf("%s\n",procTable[i].name);
		}
	}
}
