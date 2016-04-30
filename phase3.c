/*
 452 Phase 3 Part B
 Authors:
 Matt Seall
 Zachary Nixon
 */

#include <phase3.h>
#include <phase2.h>
#include <phase1.h>
#include <string.h>
#include <libuser.h>

#define VMON 1
#define VMOFF 0 

#define PAGES 0
#define FRAMES 1
#define BLOCKS 2
#define FFRAMES 3
#define FBLOCKS 4
#define SWITCHES 5
#define FAULTS 6
#define NEW 7
#define PINS 8
#define POUTS 9
#define REPLACED 10

#define SWAP_DISK_NUM 1

#define SECTORS_FOR_FRAME 8

/*
 * Everybody uses the same tag.
 */
#define TAG 0
/*
 * Page table entry
 */

#define UNUSED  0
#define INCORE  1
#define ONDISK 2
/* You'll probably want more states */

/* Page Table Entry */
typedef struct PTE {
	int state; /* See above. */
	int frame; /* The frame that stores the page. */
	int index; /*Index of frame struct*/
} PTE;

/*
 * Per-process information
 */
typedef struct Process {
	int numPages; /* Size of the page table. */
	PTE *pageTable; /* The page table for the process. */
} Process;

static Process processes[P1_MAXPROC];
static int numPages = 0;
static int numFrames = 0;



static int* frameList;
typedef struct frameStruct{
	int open; /*0 or 1*/
	int track; /*Track to find frame 0 to NUM_TRACKS - 1*/
	int startSector; /*Where to find first byte of frame on disk, either 0 or 8.*/
} frameStruct;
static frameStruct* frameStructList;
static int blocksOnDisk;
static P1_Semaphore FLSem;


/*
 * Information about page faults.
 */
typedef struct Fault {
	int pid; /* Process with the problem. */
	void *addr; /* Address that caused the fault. */
	int mbox; /* Where to send reply. */
/* Add more stuff here if necessary. */
} Fault;
int FAULT_SIZE = sizeof(Fault);


P3_VmStats P3_vmStats;
static P1_Semaphore vmStatsSem;

static int pagerMbox = -1;
static P1_Semaphore pagerRunning;
static P1_Semaphore pagerDone;
static int numPagers = -1;

static int clockHand;

static void CheckPid(int);
static void CheckMode(void);
static int CheckInit(void);
static void ModifyStats(int field,int modifier);
static void FaultHandler(int type, void *arg);
static int Pager(void *arg);
static int ClockAlgo(void);

void P3_Fork(int pid);
void P3_Switch(int old, int new);
void P3_Quit(int pid);

static int done = 0;

int P3_Startup(void *arg){
	int status;
	int pid;
	Sys_Spawn("P4_Startup", P4_Startup, NULL, 4 * USLOSS_MIN_STACK, 3,&pid);	
	Sys_Wait(&pid,&status);
	return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * P3_VmInit --
 *
 *  Initializes the VM system by configuring the MMU and setting
 *  up the page tables.
 *
 * Results:
 *      MMU return status
 *
 * Side effects:
 *      The MMU is initialized.
 *
 *----------------------------------------------------------------------
 */
int P3_VmInit(int mappings, int pages, int frames, int pagers) {
	
	int status;
	int i;
	CheckMode();
	if(mappings != pages || pagers > P3_MAX_PAGERS || pagers <= 0){
		return -1;
	}
	
	if(CheckInit() == VMON){
		return -2;
	}
	status = USLOSS_MmuInit(mappings, pages, frames);
	if (status != USLOSS_MMU_OK) {
		USLOSS_Console("P3_VmInit: couldn't initialize MMU, status %d\n",
				status);
		USLOSS_Halt(1);
	}
	USLOSS_IntVec[USLOSS_MMU_INT] = FaultHandler;
	for (i = 0; i < P1_MAXPROC; i++) {
		processes[i].numPages = 0;
		processes[i].pageTable = NULL;
	}
	pagerMbox = P2_MboxCreate(P1_MAXPROC,sizeof(Fault));
	/*
	 * Create the page fault mailbox and fork the pagers here.
	*/	 
	pagerMbox = P2_MboxCreate(P1_MAXPROC,sizeof(Fault));
        pagerRunning = P1_SemCreate(0);
	pagerDone = P1_SemCreate(0);
	numPagers = pagers;
	done = 0;
	vmStatsSem = P1_SemCreate(1);
	FLSem = P1_SemCreate(1);
	frameList = malloc(sizeof(int) * frames);	
	for(i = 0; i < frames;i++){
		frameList[i] = UNUSED;
	}
	for(i = 0; i < pagers; i++){
		P1_Fork("Pager",Pager,NULL,USLOSS_MIN_STACK * 3,P3_PAGER_PRIORITY);
		P1_P(pagerRunning);
	}
	memset(&P3_vmStats, 0, sizeof(P3_VmStats));
	ModifyStats(PAGES,pages);
	ModifyStats(FRAMES,frames);
	ModifyStats(FFRAMES,frames);
	int sector;
	int numSectors;
	int tracks;
	P2_DiskSize(SWAP_DISK_NUM,&sector,&numSectors,&tracks);
	blocksOnDisk = (sector * numSectors * tracks) / USLOSS_MmuPageSize();
	ModifyStats(FBLOCKS,blocksOnDisk);
	ModifyStats(BLOCKS,blocksOnDisk);
	numPages = pages;
	numFrames = frames;
	clockHand = 0;
	frameStructList = malloc(sizeof(frameStruct) * blocksOnDisk);
	int trackCounter = 0;
	for(i = 0; i < blocksOnDisk;i++){
		frameStructList[i].open = 0;
		frameStructList[i].track = trackCounter;
		if(i % 2 == 0){
			frameStructList[i].startSector = 0;
		}else{
			frameStructList[i].startSector = 8;
			trackCounter++;
		}
	}
	return 0;
}
/*
 *----------------------------------------------------------------------
 *
 * P3_VmDestroy --
 *
 *  Frees all of the global data structures
 *
 * Results:
 *      None
 *
 * Side effects:
 *      The MMU is turned off.
 *
 *----------------------------------------------------------------------
 */
void P3_VmDestroy(void) {
	CheckMode();
	if(CheckInit() == VMOFF){
		return;
	}
	USLOSS_MmuDone();
	/*
	 * Kill the pagers here.
	*/ 
	done = 1;
	int i;
	Fault f;
	for(i = 0; i < numPagers;i++){
		P2_MboxCondSend(pagerMbox,&f,&FAULT_SIZE);
		P1_P(pagerDone);
	}
	P2_MboxRelease(pagerMbox);
	P1_SemFree(pagerDone);
	P1_SemFree(FLSem);
	P1_SemFree(vmStatsSem);
	P1_SemFree(pagerRunning);
	pagerMbox = -1;
	free(frameList);
	free(frameStructList);
	P1_P(vmStatsSem);
	USLOSS_Console("P3_vmStats:\n");
	USLOSS_Console("pages: %d\n", P3_vmStats.pages);
	USLOSS_Console("frames: %d\n", P3_vmStats.frames);
	USLOSS_Console("blocks: %d\n", P3_vmStats.blocks);
	USLOSS_Console("freeFrames: %d\n",P3_vmStats.freeFrames);
	USLOSS_Console("freeBlocks: %d\n",P3_vmStats.freeBlocks);
	USLOSS_Console("switches: %d\n",P3_vmStats.switches);
	USLOSS_Console("faults: %d\n",P3_vmStats.faults);
	USLOSS_Console("new: %d\n",P3_vmStats.new);
	USLOSS_Console("pageIns: %d\n",P3_vmStats.pageIns);
	USLOSS_Console("pageOuts: %d\n",P3_vmStats.pageOuts);
	USLOSS_Console("replaced: %d\n",P3_vmStats.replaced);
	P1_V(vmStatsSem);
}

/*
 *----------------------------------------------------------------------
 *
 * P3_Fork --
 *
 *  Sets up a page table for the new process.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  A page table is allocated.
 *
 *----------------------------------------------------------------------
 */
void P3_Fork(int pid)
{
	int i;
	CheckMode();
	CheckPid(pid);
	if(CheckInit() == VMOFF){
		return;
	}
	processes[pid].numPages = numPages;
	processes[pid].pageTable = (PTE *) malloc(sizeof(PTE) * numPages);
	for (i = 0; i < numPages; i++) {
		processes[pid].pageTable[i].frame = -1;
		processes[pid].pageTable[i].index = -1;
		processes[pid].pageTable[i].state = UNUSED;
	}
}

/*
 *----------------------------------------------------------------------
 *
 * P3_Quit --
 *
 *  Called when a process quits and tears down the page table
 *  for the process and frees any frames and disk space used
 *      by the process.
 *
 * Results:
 *  None
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
void P3_Quit(int pid) {
	CheckMode();
	CheckPid(pid);
        if(CheckInit() == VMOFF){
                return;
        }

	/*
	 * Free any of the process's pages that are on disk and free any page frames the
	 * process is using.
	 */
	
	int i;
	int status;

	/*
	Need to handle pages on the swap disk as well. PART B.
	*/
	P1_P(FLSem);
	for(i = 0; i < numPages;i++){
		/*If this page has a frame in memory, make it unused.*/
		if(processes[pid].pageTable[i].state == INCORE){
			frameList[processes[pid].pageTable[i].frame] = UNUSED;
			status = USLOSS_MmuUnmap(TAG, i);
			if (status != USLOSS_MMU_OK) {
				USLOSS_Console("Unable to unmap page in P3_Quit\n");
				USLOSS_Halt(1);
			}		
			ModifyStats(FFRAMES,1);
		}else if(processes[pid].pageTable[i].state == ONDISK){
			int index = processes[pid].pageTable[i].index;
			frameStructList[index].open = 0;
			ModifyStats(FBLOCKS,1);
		}
	}
	P1_V(FLSem);

	/* Clean up the page table. */
	free((char *) processes[pid].pageTable);
	processes[pid].numPages = 0;
	processes[pid].pageTable = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * P3_Switch
 *
 *  Called during a context switch. Unloads the mappings for the old
 *  process and loads the mappings for the new.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  The contents of the MMU are changed.
 *
 *----------------------------------------------------------------------
 */
void P3_Switch(int old, int new)
{
        if(CheckInit() == VMOFF){
                return;
        }
	int page;
	int status;

	CheckMode();
	CheckPid(old);
	CheckPid(new);

	ModifyStats(SWITCHES,1);
	for (page = 0; page < processes[old].numPages; page++) {
		if(processes[old].pageTable == NULL){
			break;
		}
		/*
		 * If a page of the old process is in memory then a mapping
		 * for it must be in the MMU. Remove it.
		 */
		if (processes[old].pageTable[page].state == INCORE) {
			status = USLOSS_MmuUnmap(TAG, page);
			if (status != USLOSS_MMU_OK) {
				USLOSS_Console("Unable to un-map page.\n");
				USLOSS_Halt(1);	
			}
		}
	}
	for (page = 0; page < processes[new].numPages; page++) {
		if(processes[new].pageTable == NULL){
			break;
		}
		/*
		 * If a page of the new process is in memory then add a mapping
		 * for it to the MMU.
		 */
		if (processes[new].pageTable[page].state == INCORE) {
			status = USLOSS_MmuMap(TAG, page,
					processes[new].pageTable[page].frame, USLOSS_MMU_PROT_RW);
			if (status != USLOSS_MMU_OK) {
				USLOSS_Console("Unable to map page.\n");
				USLOSS_Halt(1);
			}
		}
	}
}

/*
 *----------------------------------------------------------------------
 *
 * FaultHandler
 *
 *  Handles an MMU interrupt.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  The current process is blocked until the fault is handled.
 *
 *----------------------------------------------------------------------
 */
static void FaultHandler(int type,void *arg)
{
	int cause;
	int status;
	Fault fault;
	int size;
	cause = USLOSS_MmuGetCause();
	ModifyStats(FAULTS,1);
	fault.pid = P1_GetPID();
	fault.addr = arg;
	fault.mbox = P2_MboxCreate(1, 0);
	size = sizeof(fault);
	status = P2_MboxSend(pagerMbox, &fault, &size);
	size = 0;
	status = P2_MboxReceive(fault.mbox, NULL, &size);
	status = P2_MboxRelease(fault.mbox);
}

/*
 *----------------------------------------------------------------------
 *
 * Pager
 *
 *  Kernel process that handles page faults and does page
 *  replacement.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
int Pager(void *arg) {
	Fault f;
	P1_V(pagerRunning);
	int i;
	int size = 0;
	int newFrame;
	int status;
	int startPage;
	void *startVM = USLOSS_MmuRegion(&status);
	int pageSize = USLOSS_MmuPageSize();
	int page;
	char *buffer = malloc(pageSize);
	while (1) {
		//USLOSS_Console("--->%d\n",(USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE));
		P2_MboxReceive(pagerMbox,&f,&FAULT_SIZE);
		//USLOSS_Console("After!!\n");
		if(done == 1){
			break;
		}
		page = (int)f.addr / (int)pageSize;
		startPage = (long) startVM + (page * pageSize);
		P1_P(FLSem);
		newFrame = -1;
		/*Try to find a free frame*/
		for(i = 0 ; i < numFrames; i++){
			if(frameList[i] == UNUSED){
				/*Indicate this frame is now in use.*/
				ModifyStats(FFRAMES,-1);
				newFrame = i;
				frameList[i] = INCORE;
				break;
			}
		}

		/*Couldn't find free frame - evict one*/
		if(newFrame == -1){
			newFrame = ClockAlgo();
			int access;
			if(USLOSS_MmuGetAccess(clockHand,&access) != USLOSS_MMU_OK){
				USLOSS_Console("MMU Access returned NOT OK.\n");
				USLOSS_Halt(1);
			}
			/*Write old frame to disk.*/
			int freeSpace = -1;
			if((access & USLOSS_MMU_DIRTY) > 0){
				int i;
				for(i = 0; i < blocksOnDisk;i++){
					if(frameStructList[i].open == 0){
						freeSpace = i;
						frameStructList[i].open = 1;
						ModifyStats(FBLOCKS,-1);
						break;
					}
				}
				if(freeSpace == -1){
					USLOSS_Console("Could not allocate swap space. Killing process.\n");
					P1_Quit(0);
					P1_V(FLSem);
					continue;
				}
				status = USLOSS_MmuMap(TAG, page,newFrame,USLOSS_MMU_PROT_RW);
				if(status != USLOSS_MMU_OK){
					USLOSS_Console("Unable to map. status = %d\n",status);
					USLOSS_Halt(1);
				}
				memcpy(buffer,(void *)startPage,pageSize);
				status = USLOSS_MmuUnmap(TAG, page);
				if(status != USLOSS_MMU_OK){
					USLOSS_Console("Unable to un-map. status = %d\n",status);
					USLOSS_Halt(1);
				}
				ModifyStats(POUTS,1);
				int rc = P2_DiskWrite(SWAP_DISK_NUM,frameStructList[i].track,frameStructList[i].startSector,SECTORS_FOR_FRAME - 1,buffer);
				if(rc != 0){
					USLOSS_Console("P2_DiskWrite failed!!!\n");
					USLOSS_Halt(1);
				}
			}
			int j;
			for(i = 0; i < P1_MAXPROC;i++){
				if(processes[i].pageTable == NULL){
					continue;
				}
				for(j = 0; j < numPages;j++){
					/*Found our victim!!*/
					if(processes[i].pageTable[j].state == INCORE && processes[i].pageTable[j].frame == newFrame){
						processes[i].pageTable[j].state = ONDISK;
						processes[i].pageTable[j].index = freeSpace;
						goto d;
					}
				}
			}

		}
		d:
		if(processes[f.pid].pageTable[page].state == UNUSED){
			/*Loading this mapping to the pagers MMU mapping.*/
			ModifyStats(NEW,1);
			status = USLOSS_MmuMap(TAG, page,newFrame,USLOSS_MMU_PROT_RW);
			if(status != USLOSS_MMU_OK){
				USLOSS_Console("Unable to map. status = %d\n",status);
				USLOSS_Halt(1);
			}
			memset((void *)startPage,0,pageSize);
			status = USLOSS_MmuUnmap(TAG, page);
			if(status != USLOSS_MMU_OK){
				USLOSS_Console("Unable to un-map. status = %d\n",status);
				USLOSS_Halt(1);
			}
		}else{
			/*Read page from disc into this frame.*/
			int index = processes[f.pid].pageTable->index;
			ModifyStats(PINS,1);
			int rc = P2_DiskRead(SWAP_DISK_NUM,frameStructList[index].track,frameStructList[index].startSector,SECTORS_FOR_FRAME,buffer);
			if(rc != 0){
				USLOSS_Console("P2_DiskRead failed!!\n");
				USLOSS_Halt(1);
			}
			status = USLOSS_MmuMap(TAG, page,newFrame,USLOSS_MMU_PROT_RW);
			if(status != USLOSS_MMU_OK){
				USLOSS_Console("Unable to map. status = %d\n",status);
				USLOSS_Halt(1);
			}
			startPage = (long) startVM + (page * pageSize);
			memcpy((void *)startPage,buffer,pageSize);
			status = USLOSS_MmuUnmap(TAG, page);
			if(status != USLOSS_MMU_OK){
				USLOSS_Console("Unable to un-map. status = %d\n",status);
				USLOSS_Halt(1);
			}
			frameStructList[index].open = 0;
		}
		/*Add this frame <--> page mapping to process page table*/
		processes[f.pid].pageTable[page].frame = newFrame;
		processes[f.pid].pageTable[page].state = INCORE;
		P1_V(FLSem);
		P2_MboxSend(f.mbox, NULL, &size);
	}
	P1_V(pagerDone);
	return 1;
}

/*
 * Helper routines
 */

static void CheckPid(int pid) {
	if ((pid < 0) || (pid >= P1_MAXPROC)) {
		USLOSS_Console("Invalid pid\n");
		USLOSS_Halt(1);
	}
}

static void CheckMode(void) {
	if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
		USLOSS_Console("Invoking protected routine from user mode\n");
		USLOSS_Halt(1);
	}
}

static int CheckInit(void){
	if(pagerMbox == -1){
		return VMOFF;
	}
	return VMON;
}

static void ModifyStats(int field,int modifier){
	P1_P(vmStatsSem);
	switch(field){
	case PAGES: P3_vmStats.pages += modifier;
	break;
	case FRAMES: P3_vmStats.frames += modifier;
	break;
	case BLOCKS: P3_vmStats.blocks += modifier;
	break;
	case FFRAMES: P3_vmStats.freeFrames += modifier;
	break;
	case FBLOCKS: P3_vmStats.freeBlocks += modifier;
	break;
	case SWITCHES: P3_vmStats.switches += modifier;
	break;
	case FAULTS: P3_vmStats.faults += modifier;
	break;
	case NEW: P3_vmStats.new += modifier;
	break;
	case PINS: P3_vmStats.pageIns += modifier;
	break;
	case POUTS: P3_vmStats.pageOuts += modifier;
	break;
	case REPLACED: P3_vmStats.replaced += modifier;
	break;
	}
	P1_V(vmStatsSem);	
}

static int ClockAlgo(void){
	int victim;
	int access;
	while(1){
		if(USLOSS_MmuGetAccess(clockHand,&access) != USLOSS_MMU_OK){
			USLOSS_Console("MMU Access returned NOT OK.\n");
			USLOSS_Halt(1);
		}
		if((access & USLOSS_MMU_REF) == 0){
			victim = clockHand;
			clockHand = (clockHand + 1) % numFrames;
			return victim;
		}else{
			USLOSS_MmuSetAccess(clockHand,access & ~(USLOSS_MMU_REF));
			clockHand = (clockHand + 1) % numFrames;
		}
	}
}
