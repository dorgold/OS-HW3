#include "list.h"
#include "r_mw_w_lock.h"
#include "queue.h"

#define LIST_COMMAND_START threadCounter++
#define LIST_COMMAND_END	threadCounter--;\
						if (!threadCounter) __Cleanup()


typedef struct node Node;

struct node
{
	int key;
	char data;
	Lock lock;
	Node* next;
	Node* prev;
	Bool isActive;
};

unsigned int threadCounter;
Queue delNodeQueue;
Lock dummyLock;
Lock queueLock;
Node* head = NULL;
Node* tail = NULL;


/*******************************************************************************
		Static Methods Implementation
 *******************************************************************************/

static Node* __allocateNode(int key, char data)
{
	Node* ptr = malloc(sizeof(Node));
	CHECK_NULL(ptr,NULL)
	ptr->next=NULL;
	ptr->prev=NULL;
	ptr->data = data;
	ptr->key = key;
	ptr->isActive = TRUE;
	ptr->lock = init_lock();
	return ptr;
}

static void __freeNode(Node* node)
{
	destroy_lock(node->lock);
	free(node);
}

/*
 Should be MW on dummyLock before call
 */
static void __InsertFirstNodeInList(int key, char data)
{
	upgrade_may_write_lock(dummyLock);
	Node* node = __allocateNode(key,data);
	head = tail = node;
	release_exclusive_lock(dummyLock);
}

/*
 Should be MW on dummyLock and head->lock before call
 */
static void __InsertHeadNode(int key, char data)
{
	upgrade_may_write_lock(dummyLock);
	upgrade_may_write_lock(head->lock);

	Node* node = __allocateNode(key,data);
	node->next = head;
	head->prev = node;
	Node* oldHead = head;
	head = node;

	release_exclusive_lock(dummyLock);
	release_exclusive_lock(oldHead->lock);
}

/*
 Should be MW on tail->lock before call
 */
static void __InsertTailNode(int key, char data)
{
	upgrade_may_write_lock(tail->lock);
	get_write_lock(dummyLock);

	Node* node = __allocateNode(key,data);
	node->prev = tail;
	tail->next = node;
	Node* oldTail = tail;
	tail = node;

	release_exclusive_lock(oldTail->lock);
	release_exclusive_lock(dummyLock);
}

/*
 Should be MW on leftNode and rightNode before call
 */
static void __InsertMiddleNode(int key, char data,Node* leftNode, Node* rightNode)
{
	upgrade_may_write_lock(leftNode->lock);
	upgrade_may_write_lock(rightNode->lock);

	Node* node = __allocateNode(key,data);
	node->next = rightNode;
	node->prev = leftNode;
	leftNode->next=node;
	rightNode->prev=node;

	release_exclusive_lock(leftNode->lock);
	release_exclusive_lock(rightNode->lock);

}

static void __Cleanup()
{
	get_write_lock(queueLock);

	while(size(delNodeQueue)!=0)
	{
		Node* node = (Node*)dequeue(delNodeQueue);
		__freeNode(node);
	}

	release_exclusive_lock(queueLock);
}

/*
 Should be MW on node before call
 */
__MakeActive(Node* node, char data)
{
	upgrade_may_write_lock(node->lock);
	node->isActive = TRUE;
	node->data = data;
	release_exclusive_lock(node->lock);
}

/*
 Returns a node with reading premissions or NULL on failure
 */
Node* __FindNode (int key)
{

	Node* curr;
	get_read_lock(dummyLock);
	if (head == NULL)
	{
		release_shared_lock(dummyLock);
		return NULL;
	}
	curr = head;
	release_shared_lock(dummyLock);
	get_read_lock(curr->lock);
	while (!(curr->isActive && curr->key == key))
	{
		Node* nextNode = curr->next;
		release_shared_lock(curr->lock);
		if (nextNode == NULL)
		{
			return NULL;
		}

		curr = nextNode;
		get_read_lock(curr->lock);
	}

	return curr;
}

/*
	Should be MW on curr before call.
	If insertion succeeded returns true. if key exists returns false;
 */
Bool __advanceRight(Node* curr, int key, char data)
{
	while (curr->next != NULL)
	{
		if (curr->key == key)
		{
			release_shared_lock(curr->lock);
			return FALSE;
		}

		Node* next = curr->next;
		get_may_write_lock(next->lock);
		if (next->key > key)
		{
			__InsertMiddleNode(key,data,curr,next);
			return TRUE;
		}

		release_shared_lock(curr->lock);
		curr = next;
	}

	if (curr->key == key)
	{
		release_shared_lock(curr->lock);
		return FALSE;
	}

	__InsertTailNode(key,data);
	return TRUE;
}

/*
 Should be MW on prev (dummyLock if head) and curr before call
 */
void __DeleteNode(Node* prev, Node* curr)
{
	Node* next = curr->next;
	Bool isTail = (next == NULL);
	Bool isHead = (prev == NULL);
	Lock prevLock, nextLock;
	if (!isTail)
	{
		nextLock = next->lock;
		get_may_write_lock(next->lock);
	}
	else if (isHead && isTail)
	{
		nextLock = prevLock = dummyLock;
	}
	else
	{
		nextLock = dummyLock;
		get_may_write_lock(dummyLock);
	}

	if (isHead)
	{
		prevLock = dummyLock;
	}
	else
	{
		prevLock = prev->lock;
	}

	upgrade_may_write_lock(prevLock);
	upgrade_may_write_lock(curr->lock);
	if (nextLock != prevLock) upgrade_may_write_lock(nextLock);

	if (prev==NULL)
	{
		head = next;
	}
	else
	{
		prev->next = next;
	}
	if (next==NULL)
	{
		tail = prev;
	}
	else
	{
		next->prev = prev;
	}

	enqueue(delNodeQueue,curr);

	release_exclusive_lock(prevLock);
	release_exclusive_lock(curr->lock);
	if (nextLock != prevLock) release_exclusive_lock(nextLock);
}
/*******************************************************************************
		Interface Implementation
 *******************************************************************************/

void Initialize()
{
	dummyLock = init_lock();
	queueLock =  init_lock();
	threadCounter = 0;
	delNodeQueue = init();
}

void Destroy()
{
	destroy_lock(dummyLock);
	destroy_lock(queueLock);
	destroy(delNodeQueue);
	if (head == NULL) return;
	Node* curr = head;
	while (curr->next  != NULL)
	{
		destroy_lock(curr->lock);
		Node* next = curr->next;

		free(curr);
		curr = next;
	}
	free(curr);
}

Bool InsertHead (int key, char data)
{
	LIST_COMMAND_START;

	Node* curr;
	get_may_write_lock(dummyLock);
	if (head == NULL)
	{
		__InsertFirstNodeInList(key,data);
		LIST_COMMAND_END;
		return TRUE;
	}

	get_may_write_lock(head->lock);
	if (head->key > key)
	{
		__InsertHeadNode(key,data);
		LIST_COMMAND_END;
		return TRUE;
	}

	curr = head;
	release_shared_lock(dummyLock);

	Bool result  = __advanceRight(curr,key,data);
	LIST_COMMAND_END;
	return result;
}

Bool InsertTail (int key, char data)
{
	LIST_COMMAND_START;

	Node* curr;
	get_may_write_lock(dummyLock);
	if (tail == NULL)
	{
		__InsertFirstNodeInList(key,data);
		LIST_COMMAND_END;
		return TRUE;
	}
	release_shared_lock(dummyLock);
	get_may_write_lock(tail->lock);
	if (tail->key < key)
	{
		__InsertTailNode(key,data);
		LIST_COMMAND_END;
		return TRUE;
	}

	curr = tail;

	while (curr->prev != NULL)
	{
		if (curr->key == key && curr->isActive == TRUE)
		{
			release_shared_lock(curr->lock);
			LIST_COMMAND_END;
			return FALSE;
		}

		if (curr->key < key && curr->isActive == TRUE)
		{
			Bool result  = __advanceRight(curr,key,data);
			LIST_COMMAND_END;
			return result;
		}

		Node* oldCurr = curr;
		curr = curr->prev;
		release_shared_lock(oldCurr->lock);
		get_may_write_lock(curr->lock);
	}

	if (curr->key == key)
	{
		release_shared_lock(curr->lock);
		LIST_COMMAND_END;
		return FALSE;
	}

	release_shared_lock(curr->lock);
	InsertHead(key,data);
	LIST_COMMAND_END;
	return TRUE;

}

Bool Delete (int key)
{
	LIST_COMMAND_START;

	Node* prev;
	Node* curr;
	get_may_write_lock(dummyLock);
	if (head == NULL)
	{
		release_shared_lock(dummyLock);
		LIST_COMMAND_END;
		return FALSE;
	}

	get_may_write_lock(head->lock);
	if (head->key == key)
	{
		__DeleteNode(NULL,head);
		LIST_COMMAND_END;
		return TRUE;
	}

	prev = head;
	curr = prev->next;
	release_shared_lock(dummyLock);

	while (curr != NULL)
	{
		get_may_write_lock(curr->lock);
		if (curr->key == key)
		{
			__DeleteNode(prev,curr);
			LIST_COMMAND_END;
			return TRUE;
		}

		release_shared_lock(prev->lock);
		if (curr->key > key)
		{
			release_shared_lock(curr->lock);
			LIST_COMMAND_END;
			return FALSE;
		}

		prev = curr;
		curr = curr->next;
	}

	release_shared_lock(curr->lock);
	LIST_COMMAND_END;
	return FALSE;
}

Bool Search (int key, char* data)
{
	LIST_COMMAND_START;

	Node* node = (Node*)__FindNode(key);
	if (node == NULL)
	{
		LIST_COMMAND_END;
		return FALSE;
	}
	*data = node->data;
	release_shared_lock(node->lock);
	LIST_COMMAND_END;
	return TRUE;
}

