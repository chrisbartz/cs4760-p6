//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 6


#include "sharedMemory.h"

#define DEBUG 0
#define VERBOSE 0

sem_t* open_semaphore(int createSemaphore) {
	if (DEBUG) printf("sharedMemory: Creating semaphore\n");
	if (createSemaphore)
		return sem_open(SEM_NAME, O_CREAT|O_EXCL, 0660, 1);
	else
		return sem_open(SEM_NAME, 0);
}

void close_semaphore(sem_t *sem) {
	if (DEBUG) printf("sharedMemory: closing semaphore\n");
	sem_close(sem);
}

void printPageTable(SmStruct *p_shmMsg) {

	printf("Page Table:\n");
	for (int i = 0; i < MAX_SYSTEM_MEMORY; i++) {
		if (p_shmMsg->pageStatus[i] == PAGE_STATUS_FREE)
			printf(".");
		else if (p_shmMsg->pageStatus[i] == PAGE_STATUS_OCCUPIED)
			printf("U");
		else if (p_shmMsg->pageStatus[i] == PAGE_STATUS_DIRTY)
			printf("D");
		else
			printf("X");
	}
	printf("\n");

	for (int i = 0; i < MAX_SYSTEM_MEMORY; i++) {
		if (p_shmMsg->pageTableSecondChanceBit[i] == PAGE_SECOND_CHANCE_EMPTY)
			printf(".");
		else if (p_shmMsg->pageTableSecondChanceBit[i] == PAGE_SECOND_CHANCE_RECENTLY_USED)
			printf("1");
		else if (p_shmMsg->pageTableSecondChanceBit[i] == PAGE_SECOND_CHANCE_RECLAIMABLE)
			printf("0");
		else
			printf("X");
	}
	printf("\n");
}

int pageTableIsNearLimit(SmStruct *p_shmMsg) {
	if (DEBUG) printf("sharedMemory: Checking if page table is near limit\n");
	int usedCount = 0;

	for (int i = 0; i < MAX_SYSTEM_MEMORY; i++) {
		if (p_shmMsg->pageTableSecondChanceBit[i] == PAGE_SECOND_CHANCE_RECENTLY_USED)
			usedCount++;
	}

	if (usedCount > MAX_SYSTEM_MEMORY_MAINTENANCE){
		if (DEBUG) printf("sharedMemory: Page table is near limit: %d out of %d\n", usedCount, MAX_SYSTEM_MEMORY_MAINTENANCE);
		return 1; // page table needs maintenance
	} else {
		if (DEBUG) printf("sharedMemory: Page table is not near limit: %d out of %d\n", usedCount, MAX_SYSTEM_MEMORY_MAINTENANCE);
		return 0; // page table does not need maintenance
	}
}

void pageTableMaintenance(SmStruct *p_shmMsg) {
	if (DEBUG) printf("sharedMemory: Starting page table maintenance\n");

	for (int i = 0; i < MAX_SYSTEM_MEMORY; i++) {
		if (p_shmMsg->pageTable[i] > 0) {
			p_shmMsg->pageTableSecondChanceBit[i] = PAGE_SECOND_CHANCE_RECLAIMABLE;
			p_shmMsg->pageStatus[i] = PAGE_STATUS_OCCUPIED;
		}
	}
}

int findNextReclaimableFrame(SmStruct *p_shmMsg, int *currentPageTableReference) {
	if (DEBUG) printf("sharedMemory: Finding next reclaimable frame\n");

	if (*currentPageTableReference % 5 == 0)
		printPageTable(p_shmMsg);

	if (pageTableIsNearLimit(p_shmMsg)) {	// if we are get to the 90% occupied limit
		pageTableMaintenance(p_shmMsg);		// then mark all frames reclaimable
	}

	incrementPageTableReference(currentPageTableReference);		// get next frame

	while (p_shmMsg->pageTableSecondChanceBit[*currentPageTableReference] == PAGE_SECOND_CHANCE_RECENTLY_USED) { 	// if frame is recently used
		p_shmMsg->pageTableSecondChanceBit[*currentPageTableReference] = PAGE_SECOND_CHANCE_RECLAIMABLE;			// mark it as reclaimable
		incrementPageTableReference(currentPageTableReference); 													// and move on to the next frame
	}

	return *currentPageTableReference;
}

int* incrementPageTableReference(int *currentPageTableReference) {
	if (DEBUG) printf("sharedMemory: Getting next page table reference\n");
	*currentPageTableReference = (*currentPageTableReference + 1) % MAX_SYSTEM_MEMORY;
	return currentPageTableReference;
}

void assignFrame(SmStruct *p_shmMsg, int frameId, int pid, int pidReference) {
	if (DEBUG) printf("sharedMemory: Assigning frame %d to pid %d page %d\n", frameId, pid, pidReference);

	p_shmMsg->pageTable[frameId] = pid;
	p_shmMsg->pageTableUserPageReference[frameId] = pidReference;
	p_shmMsg->pageStatus[frameId] = PAGE_STATUS_OCCUPIED;
	p_shmMsg->pageTableSecondChanceBit[frameId] = PAGE_SECOND_CHANCE_RECENTLY_USED;

}

int accessFrame(SmStruct *p_shmMsg, int pid, int pidReference, int readWrite) {
	if (DEBUG) printf("sharedMemory: Pid %d attempting to access frame with pidReference %d\n", pid, pidReference);

	for (int i = 0; i < MAX_SYSTEM_MEMORY; i++) {															// check to see if page is in memory
		if (p_shmMsg->pageTable[i] == pid && p_shmMsg->pageTableUserPageReference[i] == pidReference) {
			printf("sharedMemory: PAGE HIT: Pid %d successfully accessed a page %d in frame %d\n", pid, pidReference, i); // if found
			if (readWrite == WRITE) // only set dirty bit on write
				p_shmMsg->pageStatus[i] = PAGE_STATUS_DIRTY;
			p_shmMsg->pageTableSecondChanceBit[i] = PAGE_SECOND_CHANCE_RECENTLY_USED;
			return PAGE_HIT;
		}
	}

	printf("sharedMemory: PAGE FAULT: Pid %d accessed a page %d which had to be retrieved from disk\n", pid, pidReference);
	return PAGE_FAULT;																								// if not found
}

void freeFrames(SmStruct *p_shmMsg, int pid) {
	if (DEBUG) printf("sharedMemory: Freeing frames assigned to pid %d\n", pid);

	for (int i = 0; i < MAX_SYSTEM_MEMORY; i++) {
		if (p_shmMsg->pageTable[i] == pid) {
			p_shmMsg->pageTable[i] = 0;
			p_shmMsg->pageTableUserPageReference[i] = 0;
			p_shmMsg->pageStatus[i] = PAGE_STATUS_FREE;
			p_shmMsg->pageTableSecondChanceBit[i] = PAGE_SECOND_CHANCE_EMPTY;
		}
	}
}

int scanRequests(SmStruct *p_shmMsg) {
	if (DEBUG && VERBOSE) printf("sharedMemory: Scanning PCBs for memory requests\n");

	for (int i = 0; i < MAX_PROCESS_CONTROL_BLOCKS; i++) {
		if (p_shmMsg->pcb[i].requestedPage != PCB_NO_REQUEST) {
			if (DEBUG) printf("sharedMemory: Found a new memory request from pid %d for page %d\n", p_shmMsg->pcb[i].pid, p_shmMsg->pcb[i].requestedPage);
			return i;
		}
	}

	if (DEBUG && VERBOSE) printf("sharedMemory: Scanning PCBs for memory requests did not yield any new requests\n");
	return PCB_SCAN_NO_REQUESTS;
}

void grantRequest(SmStruct *p_shmMsg, int pcbId) {
	if (DEBUG) printf("sharedMemory: Granting memory request to PCB %d \n", pcbId);

	int request = p_shmMsg->pcb[pcbId].requestedPage;

	p_shmMsg->pcb[pcbId].requestedPage = PCB_NO_REQUEST;
	p_shmMsg->pcb[pcbId].returnedPage = request;
}

void requestMemoryPage(SmStruct *p_shmMsg, int pcbIndex, int page) {
	if (DEBUG) printf("sharedMemory: Pcb %d requesting memory page %d\n", pcbIndex, page);

	p_shmMsg->pcb[pcbIndex].requestedPage = page;

	// randomly chose to READ or WRITE
	if (getUnixTime() % 2 == 0)
		p_shmMsg->pcb[pcbIndex].requestedPageReadWrite = READ;
	else
		p_shmMsg->pcb[pcbIndex].requestedPageReadWrite = WRITE;
}


