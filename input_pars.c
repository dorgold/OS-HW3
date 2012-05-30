#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "list.h"
#include "queue.h"
#include "barrier.h"

const int MAX_WORDS = 3;
const int MAX_LEN_W = 11; //INSERT_HEAD/TAIL
const int MAX_LEN_L = 100;//lenght of line from input -always smaller then 100.



typedef enum
{
	MORE_TO_GO,
	ERROR,
	END_OF_FILE,
	BEGIN_OF_FILE,
	BARRIER,
	SECCESS
} LineCode;

//typedef enum
//{
//	FALSE,
//	TRUE
//}Bool;

typedef struct{
	char commName[12];
	char f_arg[20];
	char s_arg[20];
	int num_of_params;
}commNode;

typedef void *(*func_p_t)(void*);

int splitLine(char* command,char words[MAX_WORDS][MAX_LEN_W+1])
{
	int i=0;
	const char* delimiters = " \n\r\t";
	char *tok = strtok(command,delimiters);
	while (tok!=NULL)
	{
		strcpy(words[i++],tok);
		tok = strtok(NULL,delimiters);
	}
	return i;
}

/*hendel input-> send to split the line, make comm_node for the command ,insert to the queue */
LineCode handleLine(FILE* input,Queue q)
{
	char buffer[MAX_LEN_L];
	char* currentLine = fgets(buffer,MAX_LEN_L,input);
	if (currentLine == NULL)
	{
		printf("ERROR");
		exit(-1);
	}
	char words[MAX_WORDS][MAX_LEN_W+1];
	int parameterCount = splitLine(currentLine,words);
	if (parameterCount == 1)
	{
		if (strcmp(words[0],"BEGIN") == 0)
			return BEGIN_OF_FILE;
		if (strcmp(words[0],"END") == 0)
			return END_OF_FILE;
		if (strcmp(words[0],"BARRIER")==0)
			return BARRIER;
		return ERROR;
	}
	else if(parameterCount==2){
		commNode* new_comm = malloc(sizeof(commNode));
		CHECK_NULL(new_comm,ERROR);
		strcpy (new_comm->commName ,words[0]);
		strcpy (new_comm->f_arg ,words[1]);
		new_comm->num_of_params=2;
		enqueue(q,new_comm);
	}else if(parameterCount==3){
		commNode* new_comm = malloc(sizeof(commNode));
		CHECK_NULL(new_comm,ERROR);
		strcpy (new_comm->commName ,words[0]);
		strcpy (new_comm->f_arg, words[1]);
		strcpy (new_comm->s_arg,words[2]);
		new_comm->num_of_params=3;
		enqueue(q,new_comm);
	}else return ERROR;

	return MORE_TO_GO;
}

void* runInsertHead(void* c){
	printf("runInsertHead\n");

	commNode* command=(commNode*)c;
	barrier();
	int key=atoi(command->f_arg);
	if (key==0)
		printf("invalid key=0");
	if( InsertHead(key,*(command->s_arg))){
		printf("%s %s %s->TRUE\n",command->commName,command->f_arg,command->s_arg); //todo:output to the stndart?! use fprintf?!
	}else
		printf("%s %s %s->FALSE\n",command->commName,command->f_arg,command->s_arg);
	return (command);
}

void* runInsertTail(void* c){
	printf("runInsertTail\n");

	commNode* command=(commNode*)c;
	barrier();
	int key=atoi(command->f_arg);
	if (key==0)
			printf("invalid key=0");
	if( InsertTail(key,*(command->s_arg))){
		printf("%s %s %s->TRUE\n",command->commName,command->f_arg,command->s_arg);
	}else
		printf("%s %s %s->FALSE\n",command->commName,command->f_arg,command->s_arg);
	return(command);
}

void* runDelete(void* c){
	printf("runDelete\n");

	commNode* command=(commNode*)c;
	barrier();
	int key=atoi(command->f_arg);
	if (key==0)
			printf("invalid key=0");
	if (Delete(key)){
		printf("%s %s->TRUE\n",command->commName,command->f_arg);
	}else
		printf("%s %s->FALSE\n",command->commName,command->f_arg);
	return command;
}

void* runSearch(void* c){

	printf("runSearch\n");

	commNode* command=(commNode*)c;
	barrier();
	int key=atoi(command->f_arg);
	if (key==0)
		printf("invalid key=0");
	char data;
	if (Search(key,&data)){
		printf("%s %s->%s\n",command->commName,command->f_arg,&data);
	}else
		printf("%s %s->FALSE\n",command->commName,command->f_arg);
	return command;

}

void waitAllThreads(Queue q)
{
	while(size(q)!=0){
		pthread_t* thread = (pthread_t*)dequeue(q);
		pthread_join(*thread,NULL);
		free (thread);
	}
}

void handelCommands(Queue q){
	Queue threadQueue = init();
	while(size(q)!=0){
		pthread_t* threadPtr = malloc(sizeof(pthread_t));
		CHECK_NULL(threadPtr,NULL);
		commNode* new_command =(commNode*) dequeue(q);
		if (strcmp(new_command->commName,"INSERT_HEAD")==0)	{
			func_p_t insert_h_p = &runInsertHead;
			pthread_create(threadPtr,NULL,insert_h_p,new_command);
		} else if (strcmp(new_command->commName,"INSERT_TAIL")==0){
			func_p_t insert_t_p = &runInsertTail;
			pthread_create(threadPtr,NULL,insert_t_p,new_command);
		} else if (strcmp(new_command->commName,"DELETE")==0){
			func_p_t delete_p = &runDelete;
			pthread_create(threadPtr,NULL,delete_p,new_command);
		}else if (strcmp(new_command->commName,"SEARCH")==0){
			func_p_t search_p = &runSearch;
			pthread_create(threadPtr,NULL,search_p,new_command);
		}else printf("invalid command or split: %s",new_command->commName); //for debuging,TO DELETE
		enqueue(threadQueue,threadPtr);
	}

	waitAllThreads(threadQueue);

	destroy(threadQueue);
}



int main(void) {
	//FILE* input = stdin;
	FILE* input = fopen("/root/workspace/hw3/tests/test1.txt","r");
	Queue q = init();
	if(handleLine(input,q)!= BEGIN_OF_FILE){
		printf("the file is invalid");
		destroy(q);
		printf("FALSE1");
		return -1;
	}
	Initialize ();
	LineCode r=handleLine(input,q);
	while (r!= END_OF_FILE){
		while(r == MORE_TO_GO){
			r=handleLine(input,q);
		}
		if (r == BARRIER){
			if(size(q) == 0) break;
			barrier_init(size(q));
			handelCommands(q);

		}else if((r == ERROR) ||  (r== BEGIN_OF_FILE)){
			printf("the file is invalid");
			Destroy();
			destroy(q);
			printf("FALSE2");
			return -1;
		}
		r=handleLine(input,q);
		printf("test1");
	}
	printf("Success!!");
	Destroy();
	destroy(q);
	printf("Success!!");
	return 0;
}
