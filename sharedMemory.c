//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 6


#include "sharedMemory.h"

#define DEBUG 0

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
		else if (p_shmMsg->pageStatus[i] == PAGE_SECOND_CHANCE_CLEAN)
			printf("0");
		else if (p_shmMsg->pageStatus[i] == PAGE_SECOND_CHANCE_DIRTY)
			printf("1");
		else
			printf("X");
	}
	printf("\n");
}
