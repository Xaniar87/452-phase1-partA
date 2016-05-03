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

#define SECTORS_FOR_FRAME 8

#define SWAP_DISK_NUM 1

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
	int block; /* The disk block that stores the page. */
/* Add more stuff here */
} PTE;

typedef struct frameStruct {
	int open; /*0 or 1*/
	int track; /*Track to find frame 0 to NUM_TRACKS - 1*/
	int startSector; /*Where to find first byte of frame on disk, either 0 or 8.*/
} frameStruct;
static frameStruct* frameStructList;
static int blocksOnDisk;

/*
 * Per-process information
 */
typedef struct Process {
	int numPages; /* Size of the page table. */
	PTE *pageTable; /* The page table for the process. */
/* Add more stuff here if necessary. */
} Process;

static Process processes[P1_MAXPROC];
static int numPages = 0;
static int numFrames = 0;
static int clockHand;
static int* frameList;
static P1_Semaphore *frameSems;
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
static int numPagers = -1;
static P1_Semaphore pagerRunning;
static P1_Semaphore pagerDone;

static void CheckPid(int);
static void CheckMode(void);
static int CheckInit(void);
static void ModifyStats(int field, int modifier);
static void FaultHandler(int type, void *arg);
static int Pager(void *arg);
static int clockAlgo(void);

void P3_Fork(int pid);
void P3_Switch(int old, int new);
void P3_Quit(int pid);

static int done = 0;

int P3_Startup(void *arg) {
	int status;
	int pid;
	Sys_Spawn("P4_Startup", P4_Startup, NULL, 4 * USLOSS_MIN_STACK, 3, &pid);
	Sys_Wait(&pid, &status);
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
	if (mappings != pages || pagers > P3_MAX_PAGERS || pagers <= 0) {
		return -1;
	}

	if (CheckInit() == VMON) {
		return -2;
	}
	status = USLOSS_MmuInit(mappings, pages, frames);
        numPages = pages;
        numFrames = frames;
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
	pagerMbox = P2_MboxCreate(P1_MAXPROC, sizeof(Fault));
	/*
	 * Create the page fault mailbox and fork the pagers here.
	 */
	pagerMbox = P2_MboxCreate(P1_MAXPROC, sizeof(Fault));
	pagerRunning = P1_SemCreate(0);
	pagerDone = P1_SemCreate(0);
	numPagers = pagers;
	done = 0;
	vmStatsSem = P1_SemCreate(1);
	FLSem = P1_SemCreate(1);
	frameList = malloc(sizeof(int) * frames);
	for (i = 0; i < frames; i++) {
		frameList[i] = UNUSED;
	}
	for (i = 0; i < pagers; i++) {
		P1_Fork("Pager", Pager, NULL, USLOSS_MIN_STACK * 3, P3_PAGER_PRIORITY);
		P1_P(pagerRunning);
	}
	memset(&P3_vmStats, 0, sizeof(P3_VmStats));
	ModifyStats(PAGES, pages);
	ModifyStats(FRAMES, frames);
	ModifyStats(FFRAMES, frames);
	int sector;
	int numSectors;
	int tracks;
	P2_DiskSize(SWAP_DISK_NUM, &sector, &numSectors, &tracks);
	blocksOnDisk = (sector * numSectors * tracks) / USLOSS_MmuPageSize();
	ModifyStats(FBLOCKS, blocksOnDisk);
	ModifyStats(BLOCKS, blocksOnDisk);
	clockHand = 0;
	int trackCounter = 0;
	frameStructList = malloc(sizeof(frameStruct) * blocksOnDisk);
	for (i = 0; i < blocksOnDisk; i++) {
		frameStructList[i].open = 0;
		frameStructList[i].track = trackCounter;
		if (i % 2 == 0) {
			frameStructList[i].startSector = 0;
		} else {
			frameStructList[i].startSector = 8;
			trackCounter++;
		}
	}
	frameSems = malloc(sizeof(P1_Semaphore) * frames);
	for (i = 0; i < frames; i++) {
		frameSems[i] = P1_SemCreate(1);
	}
	USLOSS_Console("DEBUG!!\n");
	for(i = 0; i < blocksOnDisk;i++){
		USLOSS_Console("index = %d track = %d start = %d\n",i,frameStructList[i].track,frameStructList[i].startSector);
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
	if (CheckInit() == VMOFF) {
		return;
	}
	USLOSS_MmuDone();
	done = 1;
	int i;
	Fault f;
	for (i = 0; i < numPagers; i++) {
		P2_MboxCondSend(pagerMbox, &f, &FAULT_SIZE);
		P1_P(pagerDone);
	}
	for (i = 0; i < P3_vmStats.frames; i++) {
		P1_SemFree(frameSems[i]);
	}
	free(frameSems);
	P2_MboxRelease(pagerMbox);
	P1_SemFree(pagerDone);
	P1_SemFree(FLSem);
	P1_SemFree(pagerRunning);
	pagerMbox = -1;
	free(frameList);
	free(frameStructList);
	P1_P(vmStatsSem);
	USLOSS_Console("P3_vmStats:\n");
	USLOSS_Console("pages: %d\n", P3_vmStats.pages);
	USLOSS_Console("frames: %d\n", P3_vmStats.frames);
	USLOSS_Console("blocks: %d\n", P3_vmStats.blocks);
	USLOSS_Console("freeFrames: %d\n", P3_vmStats.freeFrames);
	USLOSS_Console("freeBlocks: %d\n", P3_vmStats.freeBlocks);
	USLOSS_Console("switches: %d\n", P3_vmStats.switches);
	USLOSS_Console("faults: %d\n", P3_vmStats.faults);
	USLOSS_Console("new: %d\n", P3_vmStats.new);
	USLOSS_Console("pageIns: %d\n", P3_vmStats.pageIns);
	USLOSS_Console("pageOuts: %d\n", P3_vmStats.pageOuts);
	USLOSS_Console("replaced: %d\n", P3_vmStats.replaced);
	P1_V(vmStatsSem);
	P1_SemFree(vmStatsSem);
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
void P3_Fork(int pid) {
	int i;
	CheckMode();
	CheckPid(pid);
	if (CheckInit() == VMOFF) {
		return;
	}
	processes[pid].numPages = numPages;
	processes[pid].pageTable = (PTE *) malloc(sizeof(PTE) * numPages);
	for (i = 0; i < numPages; i++) {
		processes[pid].pageTable[i].frame = -1;
		processes[pid].pageTable[i].block = -1;
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
	if (CheckInit() == VMOFF || processes[pid].pageTable == NULL) {
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
	for (i = 0; i < numPages; i++) {
		/*If this page has a frame in memory, make it unused.*/
		if (processes[pid].pageTable[i].state == INCORE) {
			frameList[processes[pid].pageTable[i].frame] = UNUSED;
			status = USLOSS_MmuUnmap(TAG, i);
			if (status != USLOSS_MMU_OK) {
				USLOSS_Console("Unable to unmap page in P3_Quit\n");
				USLOSS_Halt(1);
			}
			ModifyStats(FFRAMES,1);
		}
		int index = processes[pid].pageTable[i].block;
		if(index != -1){
			frameStructList[index].open = 0;
			ModifyStats(FBLOCKS, 1);
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
void P3_Switch(int old, int new) {
	if (CheckInit() == VMOFF) {
		return;
	}
	int page;
	int status;

	CheckMode();
	CheckPid(old);
	CheckPid(new);

	ModifyStats(SWITCHES, 1);
	for (page = 0; page < processes[old].numPages; page++) {
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
static void FaultHandler(int type, void *arg) {
	int cause;
	int status;
	Fault fault;
	int size;
	cause = USLOSS_MmuGetCause();
	ModifyStats(FAULTS, 1);
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
	int index;
	int access;
	char *buffer = malloc(pageSize);
	while (1) {
		P2_MboxReceive(pagerMbox, &f, &FAULT_SIZE);
		if (done == 1) {
			break;
		}
		P1_P(FLSem);
		/*See if this page is assigned a spot on disc*/
		/*Find a frame for this page.*/
		newFrame = -1;
		page = (int) f.addr / (int) pageSize;

		/*See if this page already has a spot on disc*/
		if (processes[f.pid].pageTable[page].block == -1) {
			for (i = 0; i < blocksOnDisk; i++) {
				if (frameStructList[i].open == 0) {
					index = i;
					frameStructList[i].open = 1;
					processes[f.pid].pageTable[page].block = i;
					USLOSS_Console("DBG: PID %d assigned block %d\n",f.pid,index);
					ModifyStats(FBLOCKS, -1);
					break;
				}
			}
			/*Out of space.*/
			if (index == -1) {
				USLOSS_Console(
						"No more space on swap disk. Killing process.\n");
				P1_Kill(f.pid);
				P1_V(FLSem);
				continue;
			}

		}
		for (i = 0; i < numFrames; i++) {
			if (frameList[i] == UNUSED) {
				/*Indicate this frame is now in use.*/
				ModifyStats(FFRAMES, -1);
				newFrame = i;
				frameList[i] = INCORE;
				break;
			}
		}

		/*No free frames*/
		if (newFrame == -1) {
			ModifyStats(REPLACED, 1);
			newFrame = clockAlgo();
			/*Evict the old page - write it to disc*/
			int j;

			/*Find the page associated with this frame - update the PTE for that process*/
			for (i = 0; i < P1_MAXPROC; i++) {
				if (processes[i].pageTable == NULL) {
					continue;
				}
				for (j = 0; j < numPages; j++) {
					if (processes[i].pageTable[j].state == INCORE
							&& processes[i].pageTable[j].frame == newFrame) {
						/*Got the PTE that holds this frame - update it.*/
						processes[i].pageTable[j].state = ONDISK;
						index = processes[i].pageTable[j].block;
						break;
					}
				}
			}

			/*Determine if we need to write to disc.*/
			int a;
			if ((a = USLOSS_MmuGetAccess(newFrame, &access)) != USLOSS_MMU_OK) {
				USLOSS_Console("MMU Access returned NOT OK. %d\n",a);
				USLOSS_Halt(1);
			}

			if ((access & USLOSS_MMU_DIRTY) > 0) {
				ModifyStats(POUTS, 1);
				status = USLOSS_MmuMap(TAG, page, newFrame, USLOSS_MMU_PROT_RW);
				if (status != USLOSS_MMU_OK) {
					USLOSS_Console("Unable to map. status = %d\n", status);
					USLOSS_Halt(1);
				}
				startPage = (long) startVM + (page * pageSize);
				memcpy(buffer, (void *) startPage, pageSize);
				status = USLOSS_MmuUnmap(TAG, page);
				if (status != USLOSS_MMU_OK) {
					USLOSS_Console("Unable to un-map. status = %d\n", status);
					USLOSS_Halt(1);
				}
				USLOSS_Console("WROTE block = %d, pid = %d,Buffer = %s\n",processes[f.pid].pageTable[page].block,f.pid,buffer);
				P1_V(FLSem);
				status = P2_DiskWrite(SWAP_DISK_NUM,
						frameStructList[index].track,
						frameStructList[index].startSector, SECTORS_FOR_FRAME,
						buffer);
				if (status != 0) {
					USLOSS_Console("P2_DiskWrite failed!!\n");
					USLOSS_Halt(1);
				}
			}
		} /*End if newFrame == -1*/
		else {
			P1_V(FLSem);
		}

		/*At this point we definitely have a frame.*/
		P1_P(frameSems[newFrame]);

		/*Page never used before - zero it.*/
		if (processes[f.pid].pageTable[page].state == UNUSED) {
			ModifyStats(NEW, 1);
			/*Loading this mapping to the pagers MMU mapping.*/
			status = USLOSS_MmuMap(TAG, page, newFrame, USLOSS_MMU_PROT_RW);
			if (status != USLOSS_MMU_OK) {
				USLOSS_Console("Unable to map. status = %d\n", status);
				USLOSS_Halt(1);
			}
			/*Zero this frame.*/
			startPage = (long) startVM + (page * pageSize);
			memset((void *) startPage, 0, pageSize);
			status = USLOSS_MmuUnmap(TAG, page);
			if (status != USLOSS_MMU_OK) {
				USLOSS_Console("Unable to un-map. status = %d\n", status);
				USLOSS_Halt(1);
			}
		} else {
			ModifyStats(PINS, 1);
			int index = processes[f.pid].pageTable[page].block;
			status = P2_DiskRead(SWAP_DISK_NUM, frameStructList[index].track,
					frameStructList[index].startSector, SECTORS_FOR_FRAME,
					buffer);
			if (status != 0) {
				USLOSS_Console("P2_DiskRead failed!!\n");
				USLOSS_Halt(1);
			}
			USLOSS_Console("READ block = %d, pid = %d,Buffer = %s\n",processes[f.pid].pageTable[page].block,f.pid,buffer);
			/*Page has been used before, so it's on disk (if it was in memory we wouldn't have this fault).*/
			status = USLOSS_MmuMap(TAG, page, newFrame, USLOSS_MMU_PROT_RW);
			if (status != USLOSS_MMU_OK) {
				USLOSS_Console("Unable to map. status = %d\n", status);
				USLOSS_Halt(1);
			}
			startPage = (long) startVM + (page * pageSize);
			memcpy((void *) startPage, buffer, pageSize);
			status = USLOSS_MmuUnmap(TAG, page);
			if (status != USLOSS_MMU_OK) {
				USLOSS_Console("Unable to un-map. status = %d\n", status);
				USLOSS_Halt(1);
			}
		}
		/*Add this frame <--> page mapping to process page table*/
		processes[f.pid].pageTable[page].frame = newFrame;
		processes[f.pid].pageTable[page].state = INCORE;
		P1_V(frameSems[newFrame]);
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
		USLOSS_Console("Invalid pid %d\n",pid);
		USLOSS_Halt(1);
	}
}

static void CheckMode(void) {
	if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
		USLOSS_Console("Invoking protected routine from user mode\n");
		USLOSS_Halt(1);
	}
}

static int CheckInit(void) {
	if (pagerMbox == -1) {
		return VMOFF;
	}
	return VMON;
}

static void ModifyStats(int field, int modifier) {
	P1_P(vmStatsSem);
	switch (field) {
	case PAGES:
		P3_vmStats.pages += modifier;
		break;
	case FRAMES:
		P3_vmStats.frames += modifier;
		break;
	case BLOCKS:
		P3_vmStats.blocks += modifier;
		break;
	case FFRAMES:
		P3_vmStats.freeFrames += modifier;
		break;
	case FBLOCKS:
		P3_vmStats.freeBlocks += modifier;
		break;
	case SWITCHES:
		P3_vmStats.switches += modifier;
		break;
	case FAULTS:
		P3_vmStats.faults += modifier;
		break;
	case NEW:
		P3_vmStats.new += modifier;
		break;
	case PINS:
		P3_vmStats.pageIns += modifier;
		break;
	case POUTS:
		P3_vmStats.pageOuts += modifier;
		break;
	case REPLACED:
		P3_vmStats.replaced += modifier;
		break;
	}
	P1_V(vmStatsSem);
}

static int clockAlgo(void) {
	int victim;
	int access;
	while (1) {
		int a;
		if ((a = USLOSS_MmuGetAccess(clockHand, &access)) != USLOSS_MMU_OK) {
			USLOSS_Console("MMU Access returned NOT OK.%d\n",a);
			USLOSS_Halt(1);
		}
		if ((access & USLOSS_MMU_REF) == 0) {
			victim = clockHand;
			clockHand = (clockHand + 1) % numFrames;
			return victim;
		} else {
			USLOSS_MmuSetAccess(clockHand, access & ~(USLOSS_MMU_REF));
			clockHand = (clockHand + 1) % numFrames;
		}
	}
}

