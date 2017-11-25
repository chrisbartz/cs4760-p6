//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 6

#ifndef SHAREDMEMORY_H_
#define SHAREDMEMORY_H_

// set up shared memory keys for communication
#define SHM_MSG_KEY 98753
#define SHMSIZE sizeof(SmStruct)
#define SEM_NAME "cyb01b_p5"
#define MAX_PROCESS_CONTROL_BLOCKS 18
#define MAX_RESOURCE_DESCRIPTORS 20
#define MAX_RESOURCE_COUNT 20
#define MAX_RESOURCE_QTY 10
#define MAX_RESOURCE_REQUEST_COUNT 1000
#define RESOURCE_DESCRIPTOR_MOD 5
#define MAX_RESOURCES 20

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
	int userHaltSignal; // 1 terminated
	int userHaltTime;
	int userResource;
	int userRequestOrRelease; // 0 none 1 request 2 release
	int userGrantedResource;
	SmProcessControlBlock pcb[MAX_PROCESS_CONTROL_BLOCKS];
	SmResourceDescriptor resDesc[MAX_RESOURCE_DESCRIPTORS];
	int resourcesGrantedCount[MAX_RESOURCE_COUNT];
	int resourceRequestQueue[MAX_RESOURCE_REQUEST_COUNT][2]; // 0: pid 1: resource request
} SmStruct;

sem_t* open_semaphore(int createSemaphore);

void close_semaphore(sem_t *sem);

#endif /* SHAREDMEMORY_H_ */
