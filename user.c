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

#define DEBUG 1 			// setting to 1 greatly increases number of logging events
#define VERBOSE 1 			// setting to 1 greatly increases number of logging events
#define TUNING 0
#define MAX_WORK_INTERVAL 75 * 1000 * 1000 // max time to work
#define BINARY_CHOICE 2
#define MAX_RESOURCE_WAIT 100 * 1000 * 1000
#define MAX_LUCKY_NUMBER 50


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

void increment_user_wait_values(int ossSeconds, int ossUSeconds, int offset);
int get_random(int modulus);

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

	long realStartTime = getUnixTime();
	long realStopTime = realStartTime + (10 * 1000); // limit user lives to 10 real seconds

	getTime(timeVal);
	if (TUNING || DEBUG)
		printf("user %s: process %d (#%d) read start time in shared memory: %d.%09d\n",
			timeVal, (int) getpid(), childId, startSeconds, startUSeconds);


	// open semaphore
	sem_t *sem = open_semaphore(0);

	struct timespec timeperiod;
	timeperiod.tv_sec = 0;
	timeperiod.tv_nsec = 5 * 10000;

	increment_user_wait_values(p_shmMsg->ossSeconds, p_shmMsg->ossUSeconds,  get_random(MAX_RESOURCE_WAIT));
	getTime(timeVal);
	if (DEBUG) printf("user %s: process %d set resource wait to: %d.%09d\n", timeVal, (int) getpid(), userWaitSeconds, userWaitUSeconds);

	while (1) { // main while loop

		nanosleep(&timeperiod, NULL); // reduce the cpu load from looping

		// check to see if a memory request has been granted
		if (p_shmMsg->pcb[pcbIndex].returnedPage != PCB_NO_REQUEST) { // then request has been granted
			getTime(timeVal);
			if (DEBUG) printf("user %s: process %d has detected a memory request for page %d has been granted\n", timeVal, (int) getpid(), p_shmMsg->pcb[pcbIndex].returnedPage);

			sem_wait(sem);
			p_shmMsg->pcb[pcbIndex].returnedPage = PCB_NO_REQUEST;	// clear the message
			sem_post(sem);
		}

		// check to see if a memory request has been made
		if (p_shmMsg->pcb[pcbIndex].requestedPage != PCB_NO_REQUEST) {
//			getTime(timeVal);
//			if (0 && DEBUG && VERBOSE) printf("user %s: process %d has detected a memory request for page %d has been made\n", timeVal, (int) getpid(), p_shmMsg->pcb[pcbIndex].requestedPage);

		} else { // check to see if a memory request should be made
			if ((getUnixTime() + (int)getpid()) % 3 == 0) { // try a better pseudo random number - had no luck with srand()
				int request = getUnixTime() % MAX_USER_SYSTEM_MEMORY;
				getTime(timeVal);
				if (DEBUG && VERBOSE) printf("user %s: process %d has made a memory request for page %d\n", timeVal, (int) getpid(), request);

				sem_wait(sem);
				requestMemoryPage(p_shmMsg, pcbIndex, request);
				sem_post(sem);
			}
		}



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

		// stop user process if we exceed a real time limit
		if (realStopTime != 0 && realStopTime < getUnixTime())
			break;



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
