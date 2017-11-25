//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 6

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include "sharedMemory.h"
#include "timestamp.h"
#include "queue.h"

#define DEBUG 0 			// setting to 1 greatly increases number of logging events
#define VERBOSE 0 			// setting to 1 greatly increases number of logging events
#define TUNING 0
#define MAX_WORK_INTERVAL 75 * 1000 * 1000 // max time to work
#define BINARY_CHOICE 2
#define MAX_RESOURCE_WAIT 100 * 1000 * 1000
#define MAX_LUCKY_NUMBER 25


SmStruct shmMsg;
SmStruct *p_shmMsg;

int childId; 				// store child id number assigned from parent
int pcbIndex;				// store index of pcb

int startSeconds;			// store oss seconds when initializing shared memory
int startUSeconds;			// store oss nanoseconds when initializing shared memory
int endSeconds;				// store oss seconds to exit
int endUSeconds;			// store oss nanoseconds to exit
int exitSeconds;			// store oss seconds when exiting
int exitUSeconds;			// store oss nanoseconds when exiting

int userWaitSeconds;		// the next time the user process makes a resource decision
int userWaitUSeconds;		// the next time the user process makes a resource decision
int luckyNumber; // a random number to determine if the process terminates
int requestedAResource = 0;

char timeVal[30]; // formatted time values for logging

void do_work(int willRunForThisLong);
void increment_user_wait_values(int ossSeconds, int ossUSeconds, int offset);
int get_random(int modulus);
void init_resources();
int count_resources();
int insert_resource(int resource);
void release_resource(int resource);

int main(int argc, char *argv[]) {
childId = atoi(argv[0]); // saves the child id passed from the parent process
pcbIndex = atoi(argv[1]); // saves the pcb index passed from the parent process

getTime(timeVal);
if (DEBUG && VERBOSE) printf("user %s: PCBINDEX: %d\n", timeVal, pcbIndex);

srand(getpid());
luckyNumber = get_random(MAX_LUCKY_NUMBER);

//int processTimeRequired = rand() % (MAX_WORK_INTERVAL);
const int oneBillion = 1000000000;

// a quick check to make sure user received a child id
getTime(timeVal);
if (childId < 0) {
	if (DEBUG) printf("user %s: Something wrong with child id: %d\n", timeVal, getpid());
	exit(1);
} else {
	if (DEBUG) printf("user %s: process %d (#%d) started normally after execl\n", timeVal, (int) getpid(), childId);

	// instantiate shared memory from oss
	getTime(timeVal);
	if (DEBUG) printf("user %s: process %d (#%d) create shared memory\n", timeVal, (int) getpid(), childId);

	// refactored shared memory using struct
	int shmid;
	if ((shmid = shmget(SHM_MSG_KEY, SHMSIZE, 0660)) == -1) {
		printf("sharedMemory: shmget error code: %d", errno);
		perror("sharedMemory: Creating shared memory segment failed\n");
		exit(1);
	}
	p_shmMsg = &shmMsg;
	p_shmMsg = shmat(shmid, NULL, 0);

	startSeconds = p_shmMsg->ossSeconds;
	startUSeconds = p_shmMsg->ossUSeconds;

	getTime(timeVal);
	if (TUNING || DEBUG)
		printf("user %s: process %d (#%d) read start time in shared memory: %d.%09d\n",
			timeVal, (int) getpid(), childId, startSeconds, startUSeconds);


	// open semaphore
	sem_t *sem = open_semaphore(0);

	struct timespec timeperiod;
	timeperiod.tv_sec = 0;
	timeperiod.tv_nsec = 5 * 10000;

	init_resources();

	increment_user_wait_values(p_shmMsg->ossSeconds, p_shmMsg->ossUSeconds,  get_random(MAX_RESOURCE_WAIT));
	getTime(timeVal);
	if (DEBUG) printf("user %s: process %d set resource wait to: %d.%09d\n", timeVal, (int) getpid(), userWaitSeconds, userWaitUSeconds);

	while (1) { // main while loop

		nanosleep(&timeperiod, NULL); // reduce the cpu load from looping

		// make decision about whether to terminate successfully
		if (!(p_shmMsg->ossSeconds >= userWaitSeconds && p_shmMsg->ossUSeconds > userWaitUSeconds)) {
				nanosleep(&timeperiod, NULL); // reduce the cpu load from looping
			} else {
				// make decision about whether to terminate
				int guess = (get_random(MAX_LUCKY_NUMBER));
				if (guess == luckyNumber) {
					getTime(timeVal);
					if (DEBUG) printf("user %s: process %d determined that guess = %d and luckyNumber = %d and it is time to terminate at %d.%09d\n",
							timeVal, (int) getpid(), guess, luckyNumber, p_shmMsg->ossSeconds, p_shmMsg->ossUSeconds);
					break;
				} else {
					if (VERBOSE && DEBUG) printf("user %s: process %d determined that guess = %d and luckyNumber = %d and it is NOT time to terminate at %d.%09d\n",
								timeVal, (int) getpid(), guess, luckyNumber, p_shmMsg->ossSeconds, p_shmMsg->ossUSeconds);
					increment_user_wait_values(p_shmMsg->ossSeconds, p_shmMsg->ossUSeconds,  get_random(MAX_RESOURCE_WAIT));
				}

			}

		// check for requested resource
		if (requestedAResource) {

			getTime(timeVal);
			if (DEBUG && VERBOSE) printf("user %s: process %d checking on resource %d reply from OSS\n", timeVal, (int) getpid(), p_shmMsg->userResource);

			if (p_shmMsg->userPid != (int) getpid() || p_shmMsg->userGrantedResource == 0) {
				// resource has not been granted (yet)
//				continue;
			} else {
				// then resource has been granted
				insert_resource(p_shmMsg->userGrantedResource);

				getTime(timeVal);
				printf("user %s: Receiving that process %d has been granted resource %d\n", timeVal, (int) getpid(), p_shmMsg->userGrantedResource);

				// keep track of assigned resources
				if (!insert_resource(p_shmMsg->userGrantedResource)) {
					getTime(timeVal);
					printf("user %s: process %d cannot accept resource %d (resource queue full)\n", timeVal, (int) getpid(), p_shmMsg->userGrantedResource);
				}

				// clear message
				sem_wait(sem);
				p_shmMsg->userPid = 0;
				p_shmMsg->userRequestOrRelease = 0;
				p_shmMsg->userResource = 0;
				p_shmMsg->userGrantedResource = 0;
				sem_post(sem);
				requestedAResource = 0;
			}
		}

		// make decision about whether to request a resource
		if ((get_random(MAX_LUCKY_NUMBER)) == luckyNumber) {

				if (get_random(BINARY_CHOICE)) { // request a resource
					int resource = get_random(MAX_RESOURCE_COUNT) + 1;
					sem_wait(sem);
					if (p_shmMsg->userPid != 0)
						sem_post(sem);
					else {
						p_shmMsg->userRequestOrRelease = 1;
						p_shmMsg->userResource = resource;
						p_shmMsg->userPid = getpid();
						sem_post(sem);
						requestedAResource = 1;
						getTime(timeVal);
						if (DEBUG) printf("user %s: process %d has requested resource %d at %d.%09d\n", timeVal, (int) getpid(), resource, p_shmMsg->ossSeconds, p_shmMsg->ossUSeconds);
					}

			}

			else { // release a resource
				int releasedResource = 0;

				// first find a resource to release
				for (int i = 0; i < MAX_RESOURCES; i++) {
					if (p_shmMsg->pcb[pcbIndex].resources[i] != 0) {
						releasedResource = p_shmMsg->pcb[pcbIndex].resources[i];
						break;
					}
				}

				// send order to release resource
				if (releasedResource) {

					release_resource(releasedResource);

					sem_wait(sem);
					if (p_shmMsg->userPid != 0)
						sem_post(sem);
					else {
						p_shmMsg->userRequestOrRelease = 2;
						p_shmMsg->userResource = releasedResource;
						p_shmMsg->userPid = getpid();
						sem_post(sem);

						getTime(timeVal);
						printf("user %s: process %d has released resource %d\n", timeVal, (int) getpid(), releasedResource);
						}

				}
			}
		}

	} // end main while loop

	getTime(timeVal);
	printf("user %s: process %d escaped main while loop at %d.%09d\n", timeVal, (int) getpid(), p_shmMsg->ossSeconds, p_shmMsg->ossUSeconds);

	sem_wait(sem);

	// send total bookkeeping stats
	p_shmMsg->pcb[pcbIndex].startUserSeconds = startSeconds;
	p_shmMsg->pcb[pcbIndex].startUserUSeconds = startUSeconds;
	p_shmMsg->pcb[pcbIndex].endUserSeconds = p_shmMsg->ossSeconds;
	p_shmMsg->pcb[pcbIndex].endUserUSeconds = p_shmMsg->ossUSeconds;

	// report process termination to oss
	p_shmMsg->userHaltSignal = 1;
	p_shmMsg->userPid = (int) getpid();

	sem_post(sem);

	// clean up shared memory
	shmdt(p_shmMsg);

	// close semaphore
	close_semaphore(sem);

	getTime(timeVal);
	if (DEBUG) printf("user %s: process %d (#%d) exiting normally\n", timeVal, (int) getpid(), childId);
}
exit(0);
}


// this part should occur within the critical section if
// implemented correctly since it accesses shared resources
void do_work(int willRunForThisLong) {

	getTime(timeVal);
	printf("user %s: process %d doing work for %d nanoseconds\n", timeVal, (int) getpid(), willRunForThisLong);

	struct timespec sleeptime;
	sleeptime.tv_sec = 0;
	sleeptime.tv_nsec = willRunForThisLong;
	nanosleep(&sleeptime, NULL); // we are doing "work" here

	getTime(timeVal);
	printf("user %s: process %d done doing work\n", timeVal, (int) getpid());

}

void increment_user_wait_values(int ossSeconds, int ossUSeconds, int offset) {
	const int oneBillion = 1000000000;

	userWaitSeconds = ossSeconds;
	userWaitUSeconds = ossUSeconds;

	userWaitUSeconds += offset;

	if (userWaitUSeconds >= oneBillion) {
		userWaitSeconds++;
		userWaitUSeconds -= oneBillion;
	}

	if (VERBOSE && DEBUG) printf("user: updating user wait time values by %d ms to %d.%09d\n", offset, userWaitSeconds, userWaitUSeconds);
}

int get_random(int modulus) {
	return rand() % modulus;
}

int insert_resource(int resource) {
	int status = 0;
	if (VERBOSE && DEBUG) printf("user: process %d resource block (request): ", (int) getpid());
	for (int i = 0; i < MAX_RESOURCES; i++) {
		if (status == 0 && p_shmMsg->pcb[pcbIndex].resources[i] == 0) {
			p_shmMsg->pcb[pcbIndex].resources[i] = resource;
			status = 1;
			if (VERBOSE && DEBUG) printf("%d,", p_shmMsg->pcb[pcbIndex].resources[i]);
		} else {
			if (VERBOSE && DEBUG) printf("%d,", p_shmMsg->pcb[pcbIndex].resources[i]);
		}
	}
	if (VERBOSE && DEBUG) printf("\n");
	return status;
}

void release_resource(int resource) {
	int status = 0;
	if (VERBOSE && DEBUG) printf("user: process %d resource block (release): ", (int) getpid());
	for (int i = 0; i < MAX_RESOURCES; i++) {
		if (VERBOSE && DEBUG) printf("%d,", p_shmMsg->pcb[pcbIndex].resources[i]);
		if (status == 0 && p_shmMsg->pcb[pcbIndex].resources[i] == resource) {
			p_shmMsg->pcb[pcbIndex].resources[i] = 0;
			status = 1;
			int resourceIndex = (int) (resource/100);
			if (1|| VERBOSE && DEBUG) printf("user: process %d decrementing resource index: %d\n", (int) getpid(), resourceIndex);
			p_shmMsg->resourcesGrantedCount[resourceIndex]--;
		}
	}
	if (VERBOSE && DEBUG) printf("\n");
}

void init_resources() {
	for (int i = 0; i < MAX_RESOURCES; i++) {
		p_shmMsg->pcb[pcbIndex].resources[i] = 0;
	}
}

int count_resources() {
	int count = 0;
	for (int i = 0; i < MAX_RESOURCES; i++) {
		if (p_shmMsg->pcb[pcbIndex].resources[i] != 0)
			count++;
	}
	return count;
}
