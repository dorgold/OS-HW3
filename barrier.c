#include "barrier.h"
#include <pthread.h>

int counter = 0;
pthread_cond_t condition;
pthread_mutex_t lock;
int isInitialized = 0;

void barrier_init(int n)
{
	if (isInitialized == 0)
	{
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_ERRORCHECK_NP);
		pthread_mutex_init(&lock, &attr);
		pthread_mutexattr_destroy(&attr);

		pthread_cond_init(&condition, NULL);

		isInitialized = 1;
	}
	counter = n;
}

void barrier()
{
	pthread_mutex_lock(&lock);
	counter--;
	if (counter > 0)
		pthread_cond_wait(&condition, &lock);
	else
		pthread_cond_broadcast(&condition);

	pthread_mutex_unlock(&lock);
}
