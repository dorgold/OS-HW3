#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct Node_s Node;
struct Node_s{
	Element data;
	Node* next;
	Node* prev;
};

struct Queue_t{
	int size;
	Node* head;
	Node* tail;
};

Queue init_queue() {
	Queue q = malloc(sizeof(struct Queue_t));
	if (q == NULL){
		printf("error allocating queue");
		exit(-1);
	}

	q->size =0;
	q->head = NULL;
	q->tail = NULL;

	return q;
}

void destroy_queue(Queue q){
	while (q->size > 0){
		dequeue(q);
	}

	free (q);
}

Queue enqueue(Queue q, Element e){

	Node* node_p = malloc (sizeof(Node));
	if (node_p == NULL) {
		printf ("error allocating node");
		exit(-1);
	}
	node_p->data = e;
	node_p->next = q->head;
	node_p->prev = NULL;
	if (q->size == 0){
		q->head=node_p;
		q->tail=q->head;
	}
	else {
		q->head->prev = node_p;
		q->head = node_p;
	}
	q->size++;

	return q;
}

Element dequeue(Queue q){

	Element temp = q->tail->data;
	Node* remove_node_p = q->tail;
	q->tail = q->tail->prev;
	q->size--;
	if (q->size != 0) {
		q->tail->next = NULL;
	}
	free (remove_node_p);
	return temp;
}


int size(Queue q){
	return q->size;
}

Element lookUp(Queue q) {
    //assuming q is not empty.
    return q->tail->data;
}
