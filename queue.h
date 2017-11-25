//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 6

#ifndef QUEUE_H_
#define QUEUE_H_

#include "sharedMemory.h"

int pop(int queue[]);

int peek(int queue[]);

void push_back(int queue[], int pushValue);

void initialize(int queue[]);

void printQueue(int queue[]);

#endif /* QUEUE_H_ */
