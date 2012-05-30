#ifndef QUEUE_H_
#define QUEUE_H_
/*
 * queue
 *
 *  Created on: May 21, 2012
 *      Author: dini
 */
typedef void* Element;
typedef struct Queue_t *Queue;

Queue enqueue(Queue q, Element e);

Element dequeue(Queue q);

Element lookUp(Queue q);

int size(Queue q);

Queue init_queue();

void destroy_queue(Queue q);

#endif
