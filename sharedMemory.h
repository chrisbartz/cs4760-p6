//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 5

#ifndef SHAREDMEMORY_H_
#define SHAREDMEMORY_H_

// set up shared memory keys for communication
#define SHM_MSG_KEY 98753
#define SHMSIZE sizeof(SmStruct)
#define SEM_NAME "cyb01b_p5"
#define MAX_PROCESS_CONTROL_BLOCKS 18
#define MAX_RESOURCE_DESCRIPTORS 20
#define MAX_RESOURCE_COUNT 10
#define RESOURCE_DESCRIPTOR_MOD 5

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
	int resourceId;
} SmResourceDescriptorInstance;

typedef struct {
	int request;
	int allocation;
	int release;
	int sharable;
	SmResourceDescriptorInstance resInstances[MAX_RESOURCE_COUNT];
} SmResourceDescriptor;

typedef struct {
	int startUserSeconds;
	int startUserUSeconds;
	int endUserSeconds;
	int endUserUSeconds;
	int totalCpuTime;
	int totalTimeInSystem;
	int lastBurstLength;
	int processPriority;
	int pid;
	int resources[100];
} SmProcessControlBlock;

typedef struct {
	int ossSeconds;
	int ossUSeconds;
//	int dispatchedPid;
//	int dispatchedTime;
	int userPid;
	int userHaltSignal; // 0 terminated 1 halted
	int userHaltTime;
	int userResource;
	int userRequestOrRelease; // 0 request 1 release
	int userGrantedResource;
	SmProcessControlBlock pcb[MAX_PROCESS_CONTROL_BLOCKS];
	SmResourceDescriptor resDesc[MAX_RESOURCE_DESCRIPTORS];
} SmStruct;

sem_t* open_semaphore(int createSemaphore);

void close_semaphore(sem_t *sem);

#endif /* SHAREDMEMORY_H_ */
