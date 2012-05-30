#include <stdlib.h>

#define CHECK_NULL(input,returnValue) \
	if (input==NULL)\
	{\
		return returnValue;\
	}

typedef enum {FALSE,TRUE} Bool;

void Initialize();
void Destroy();
Bool InsertHead (int key, char data);
Bool InsertTail (int key, char data);
Bool Delete (int key);
Bool Search (int key, char* data);
