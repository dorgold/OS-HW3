/*
 * lock_test2.c
 *
 *  Created on: May 23, 2012
 *      Author: root
 */

#include "r_mw_w_lock.h"
#include "errorcheck_mutex.h"
#include <semaphore.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#define NUM_THREADS 9

void* read_start(void*);
void* maywrite_start(void*);

Lock lock;
bool bools[NUM_THREADS];
pthread_t workers[NUM_THREADS];

pthread_cond_t cond;
pthread_cond_t work_cond;
pthread_mutex_t mutex;
int counter = NUM_THREADS;

int main() {
	lock = init_lock();
	pthread_cond_init(&cond, NULL);
	pthread_cond_init(&work_cond, NULL);
	errorcheck_mutex_init(&mutex);
	memset(bools, false, NUM_THREADS);

	int i;
	for (i = 0; i < NUM_THREADS - 1; ++i) {
		pthread_create(workers + i, NULL, read_start, (void*)i);
	}
	pthread_create(workers + i, NULL, maywrite_start, (void*)i);

	pthread_mutex_lock(&mutex);
	while(counter != 0) {
		pthread_cond_wait(&cond, &mutex);
	}
	pthread_mutex_unlock(&mutex);

	pthread_cond_broadcast(&work_cond);

	for (i = 0; i < NUM_THREADS; ++i) {
		if (!bools[i]) {
			printf("Failed\n");
		}
	}

	printf("PASS\n");
	pthread_cond_destroy(&cond);
	pthread_cond_destroy(&work_cond);
	pthread_mutex_destroy(&mutex);
	destroy_lock(lock);
	return 0;
}

void* read_start(void* v_index) {
	int index = (int)v_index;
	get_read_lock(lock);

	pthread_mutex_lock(&mutex);
	counter--;
	bools[index] = true;
	if (counter == 0) {
		pthread_cond_signal(&cond);
	}
	pthread_cond_wait(&work_cond, &mutex);
	pthread_mutex_unlock(&mutex);

	release_shared_lock(lock);

	return NULL;
}

void* maywrite_start(void* v_index) {
	int index = (int)v_index;
	get_may_write_lock(lock);

	pthread_mutex_lock(&mutex);
	counter--;
	bools[index] = true;
	if (counter == 0) {
		pthread_cond_signal(&cond);
	}
	pthread_cond_wait(&work_cond, &mutex);
	pthread_mutex_unlock(&mutex);

	release_shared_lock(lock);

	return NULL;
}
