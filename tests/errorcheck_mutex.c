/*
 * errorcheck_mutex.c
 *
 *  Created on: May 23, 2012
 *      Author: root
 */

#include "errorcheck_mutex.h"

//TODO: fix arabian mutex initialize
void errorcheck_mutex_init(pthread_mutex_t* lock) {
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
	//attr.__mutexkind = PTHREAD_MUTEX_ERRORCHECK_NP;
	pthread_mutex_init(lock, &attr);
	pthread_mutexattr_destroy(&attr);
}
