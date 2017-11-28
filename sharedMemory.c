//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 6


#include "sharedMemory.h"

#define DEBUG 1

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
		if (p_shmMsg->pageStatus[i] == PAGE_SECOND_CHANCE_EMPTY)
			printf(".");
		else if (p_shmMsg->pageStatus[i] == PAGE_SECOND_CHANCE_RECENTLY_USED)
			printf("1");
		else if (p_shmMsg->pageStatus[i] == PAGE_SECOND_CHANCE_RECLAIMABLE)
			printf("0");
		else
			printf("X");
	}
	printf("\n");
}

int pageTableIsNearLimit(SmStruct *p_shmMsg) {
	if (DEBUG) printf("sharedMemory: Checking if page table is near limit\n");
	int occupiedCount = 0;

	for (int i = 0; i < MAX_SYSTEM_MEMORY; i++) {
		if (p_shmMsg->pageTable[i] > 0)
			occupiedCount++;
	}

	if (occupiedCount > MAX_SYSTEM_MEMORY_MAINTENANCE){
		if (DEBUG) printf("sharedMemory: Page table is near limit: %d out of %d\n", occupiedCount, MAX_SYSTEM_MEMORY_MAINTENANCE);
		return 1; // page table needs maintenance
	} else {
		if (DEBUG) printf("sharedMemory: Page table is not near limit\n");
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
	if (DEBUG) printf("sharedMemory: Assigning frame %d to pid %d\n", frameId, pid);

	p_shmMsg->pageTable[frameId] = pid;
	p_shmMsg->pageTableUserPageReference[frameId] = pidReference;
	p_shmMsg->pageStatus[frameId] = PAGE_STATUS_OCCUPIED;
	p_shmMsg->pageTableSecondChanceBit[frameId] = PAGE_SECOND_CHANCE_RECENTLY_USED;

}

int accessFrame(SmStruct *p_shmMsg, int pid, int pidReference) {
	if (DEBUG) printf("sharedMemory: Pid %d attempting to access frame with pidReference %d\n", pid, pidReference);

	for (int i = 0; i < MAX_SYSTEM_MEMORY; i++) {															// check to see if page is in memory
		if (p_shmMsg->pageTable[i] == pid && p_shmMsg->pageTableUserPageReference[i] == pidReference) {
			printf("sharedMemory: PAGE HIT: Pid %d successfully accessed a page that is assigned pidReference %d\n", pid, pidReference); // if found
			p_shmMsg->pageTableSecondChanceBit[i] = PAGE_SECOND_CHANCE_RECENTLY_USED;
			return 1;
		}
	}

	printf("sharedMemory: PAGE FAULT: Pid %d attempted to access a page with pidReference %d which had to be retrieved from disk\n", pid, pidReference);
	return 0;																								// if not found
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
