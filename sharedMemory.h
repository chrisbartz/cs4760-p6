//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 6

#ifndef SHAREDMEMORY_H_
#define SHAREDMEMORY_H_

// set up shared memory keys for communication
#define SHM_MSG_KEY 98753
#define SHMSIZE sizeof(SmStruct)
#define SEM_NAME "cyb01b_p6"
#define MAX_PROCESS_CONTROL_BLOCKS 18
#define MAX_SYSTEM_MEMORY 256
#define MAX_USER_SYSTEM_MEMORY 32
#define SYSTEM_MEMORY_PAGE 1
#define NO_PAGE_WAIT 10
#define DISK_WAIT (15*1000*1000)
#define MAX_SYSTEM_MEMORY_MAINTENANCE (MAX_SYSTEM_MEMORY*.9)
#define PAGE_STATUS_FREE 0
#define PAGE_STATUS_OCCUPIED 1
#define PAGE_STATUS_DIRTY 2
#define PAGE_SECOND_CHANCE_EMPTY 0
#define PAGE_SECOND_CHANCE_CLEAN 1
#define PAGE_SECOND_CHANCE_DIRTY -1

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

typedef struct {
	int startUserSeconds;
	int startUserUSeconds;
	int endUserSeconds;
	int endUserUSeconds;
	int totalCpuTime;
	int totalTimeInSystem;
	int lastBurstLength;
	int requestedMemory;
	int pid;
	int pages[MAX_USER_SYSTEM_MEMORY];
} SmProcessControlBlock;

typedef struct {
	int ossSeconds;
	int ossUSeconds;
	int userPid;
	int userHaltSignal; // 1 terminated
	int userHaltTime;
	int pageTable[MAX_SYSTEM_MEMORY]; 						// stores the value of the user pid
	int pageTableUserPageReference[MAX_SYSTEM_MEMORY];		// stores the user pid page reference
	int pageStatus[MAX_SYSTEM_MEMORY];						// stores the status of the pageTable
	int pageTableSecondChanceBit[MAX_SYSTEM_MEMORY];
	SmProcessControlBlock pcb[MAX_PROCESS_CONTROL_BLOCKS];
} SmStruct;

sem_t* open_semaphore(int createSemaphore);

void close_semaphore(sem_t *sem);

void printPageTable();

#endif /* SHAREDMEMORY_H_ */
