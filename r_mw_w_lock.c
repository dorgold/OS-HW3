#include "r_mw_w_lock.h"
#include <pthread.h> //maybe should be more specific include address
#include <semaphore.h>
#include <assert.h>
#include "queue.h"

//TODO: linkage with pthread if needed
//TODO: call exit(-1) when malloc() returns NULL

/////my-tmp-impl/////

// Structs and typedefs
typedef enum {
	FALSE, TRUE
} Bool;

typedef enum {
	READ, MAY_WRITE, WRITE
} Mode;

typedef struct {
	pthread_cond_t cond;
	Mode mode;
} QueueElem;

struct lock {
	int num_of_readers;
	int num_of_writers;
	int num_of_may_writers;
	int num_of_wait_upgrade_requests;
	pthread_mutex_t global_lock;
	Queue wait_queue;
	pthread_cond_t wait_upgrade_request;
	pthread_t may_write_id;
};
///////////////////////////////

// Static Methods

static Bool check_read(Lock l) {
	return (l->num_of_writers > 0 || l->num_of_wait_upgrade_requests > 0);
}

static Bool check_may_write(Lock l) {
	return (l->num_of_writers > 0 || l->num_of_may_writers > 0
			|| l->num_of_wait_upgrade_requests > 0);
}

static Bool check_write(Lock l) {
	return (l->num_of_writers > 0 || l->num_of_readers > 0
			|| l->num_of_may_writers > 0 || l->num_of_wait_upgrade_requests > 0);
}

static Bool check_upgrade(Lock l) {
	return (l->num_of_readers > 0);
}

static Bool is_may_right_mode(Lock l) {
	//return (l->num_of_may_writers > 0 && pthread_equal(l->may_write_id, pthread_self()));
	if (l->num_of_may_writers > 0) {
		return pthread_equal(l->may_write_id, pthread_self());
	}
	return FALSE;
}

static Bool is_queue_empty(Lock l) {
	return (size(l->wait_queue) == 0);
}

static void put_on_wait_queue(Lock l, Mode mode) {
	QueueElem* elem = malloc(sizeof(QueueElem));
	if (elem == NULL) {
	    printf("error allocating QueueElem");
		exit(-1);
	}
	//sem_init(&elem->sem, 0, 0);
	pthread_cond_init(&elem->cond, NULL);
	elem->mode = mode;
	enqueue(l->wait_queue, elem);
	pthread_cond_wait(&elem->cond,&l->global_lock);
}

static void release_wait_queue(Lock l) {
	Bool flag = TRUE;
	QueueElem* elem;

	while (flag && !is_queue_empty(l)) {
		elem = (QueueElem*) lookUp(l->wait_queue);
		switch (elem->mode) {
		case READ:
			if (check_read(l)) {
				flag = FALSE;
				break;
			}
			pthread_cond_broadcast(&elem->cond);
			pthread_cond_destroy(&elem->cond);
			dequeue(l->wait_queue);
			free(elem);
			break;

		case MAY_WRITE:
			if (check_may_write(l)) {
				flag = FALSE;
				break;
			}
			pthread_cond_broadcast(&elem->cond);
			pthread_cond_destroy(&elem->cond);
			dequeue(l->wait_queue);
			free(elem);
			break;

		case WRITE:
			if (check_write(l)) {
				flag = FALSE;
				break;
			}
			pthread_cond_broadcast(&elem->cond);
			pthread_cond_destroy(&elem->cond);
			dequeue(l->wait_queue);
			free(elem);
			break;
		}
	}
}
/////////////////////

Lock init_lock() {
	Lock l = malloc(sizeof(struct lock));
	if (l == NULL) {
	    printf("error allocating Lock");
		exit(-1);
	}

	l->num_of_readers = 0;
	l->num_of_writers = 0;
	l->num_of_wait_upgrade_requests = 0;

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_ERRORCHECK_NP);
	pthread_mutex_init(&l->global_lock, &attr);
	pthread_mutexattr_destroy(&attr);

	l->wait_queue = init_queue();
	pthread_cond_init(&l->wait_upgrade_request, NULL);
	//l->may_write_id = 0; //we aren't intersted in the value while there is no active may_writer

	return l;
}

void get_read_lock(Lock l) {
	pthread_mutex_lock(&l->global_lock);
	if (check_read(l) || !is_queue_empty(l)) {
		put_on_wait_queue(l, READ);
	}
	l->num_of_readers++;
	pthread_mutex_unlock(&l->global_lock);
}

void get_may_write_lock(Lock l) {
	pthread_mutex_lock(&l->global_lock);
	if (check_may_write(l) || !is_queue_empty(l)) {
		put_on_wait_queue(l, MAY_WRITE);
	}
	l->may_write_id = pthread_self();
	l->num_of_may_writers++;
	pthread_mutex_unlock(&l->global_lock);
}

void get_write_lock(Lock l) {
	pthread_mutex_lock(&l->global_lock);
	if (check_write(l) || !is_queue_empty(l)) {
		put_on_wait_queue(l, WRITE);
	}
	l->num_of_writers++;
	pthread_mutex_unlock(&l->global_lock);
}

void upgrade_may_write_lock(Lock l) {
	//should be called only from the may_write mode
	pthread_mutex_lock(&l->global_lock);
	assert(is_may_right_mode(l));
	l->num_of_wait_upgrade_requests++;
	if (check_upgrade(l)) {
		pthread_cond_wait(&l->wait_upgrade_request,&l->global_lock);
	}
	l->num_of_wait_upgrade_requests--;
	l->num_of_may_writers--;
	l->num_of_writers++; //we consider upgraded may_writer as a writer
	pthread_mutex_unlock(&l->global_lock);
}

void release_shared_lock(Lock l) {
	//should be called by the thread holding the lock in
	//the read mode or in the may-write mode that wasn't upgraded
	pthread_mutex_lock(&l->global_lock);
	assert(l->num_of_readers > 0 || is_may_right_mode(l));
	if (is_may_right_mode(l)) {
		l->num_of_may_writers--; //now, num_of_may_writers = 0.
		release_wait_queue(l);
	} else {
		l->num_of_readers--;
	}

	if (l->num_of_readers == 0) {
		if (l->num_of_wait_upgrade_requests == 0) {
			release_wait_queue(l);
		} else {
			pthread_cond_broadcast(&l->wait_upgrade_request);
		}
	}
	pthread_mutex_unlock(&l->global_lock);

}

void release_exclusive_lock(Lock l) {
	//should be called by the thread holding the lock in the
	//write mode or by the upgraded may-write mode
	pthread_mutex_lock(&l->global_lock);
	assert(l->num_of_writers > 0);
	l->num_of_writers--;
	release_wait_queue(l);
	pthread_mutex_unlock(&l->global_lock);

}

void destroy_lock(Lock l) {
	pthread_mutex_destroy(&l->global_lock); //fails if mutex is locked
	destroy_queue(l->wait_queue);
	pthread_cond_destroy(&l->wait_upgrade_request);

	free(l);
}
////end-of-impl////
