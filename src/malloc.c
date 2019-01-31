#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <pthread.h>
#include "/home/ashish/WORKSPACE/Code_Examples/custom_malloc/include/malloc.h"

// --- Global variables

chunkStatus *head = NULL;
chunkStatus *lastVisited = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *brkPoint0 = NULL;

/* findChunk: search for the first chunk that fits (size equal or more) the request
 *               of the user.
 *                    chunkStatus *headptr: pointer to the first block of memory in the heap
 *                         unsigned int size: size requested by the user
 *                              retval: a poiter to the block which fits the request 
 *                              	     or NULL, in case there is no such block in the list
 *                              	     */
chunkStatus* findChunk(chunkStatus *headptr, unsigned int size)
{
	chunkStatus* ptr = headptr;

	while(ptr != NULL)
	{
		if(ptr->size >= (size + STRUCT_SIZE) && ptr->available == 1)
		{
			return ptr;
		}
		lastVisited = ptr;
		ptr = ptr->next;
	}  
	return ptr;  
}


/* splitChunk: split one big block into two. The first will have the size requested by the user.
 *   	       the second will have the remainder.
 *   	            chunkStatus* ptr: pointer to the block of memory which is going to be splitted.
 *   	                 unsigned int size: size requested by the user
 *   	                      retval: void, the function modifies the list
 *   	                      */
void splitChunk(chunkStatus* ptr, unsigned int size)
{
	chunkStatus *newChunk;	

	newChunk = (chunkStatus *)ptr->end + size;
	newChunk->size = ptr->size - size - STRUCT_SIZE;
	newChunk->available = 1;
	newChunk->next = ptr->next;
	newChunk->prev = ptr;

	if((newChunk->next) != NULL)
	{      
		(newChunk->next)->prev = newChunk;
	}

	ptr->size = size;
	ptr->available = 0;
	ptr->next = newChunk;
}


/* inscreaseAllocation: increase the amount of memory available in the heap, chaging its breakpoint
 *      chunkStatus* ptr: pointer to the block of memory which is going to be splitted.
 *           unsigned int size: size requested by the user
 *                retval: void, the function modifies the list
 *                */
chunkStatus* increaseAllocation(chunkStatus *lastVisitedPtr, unsigned int size)
{
	brkPoint0 = (chunkStatus *)sbrk((intptr_t)0);
	chunkStatus* curBreak = brkPoint0;		

	if(sbrk(MULTIPLIER * (size + STRUCT_SIZE)) == (void*) -1)
	{
		perror("SBRK");
		return NULL;
	}
	else {
		printf("sbrk called sucessfully with size: %d\n", MULTIPLIER * (size + STRUCT_SIZE));
	}

	curBreak->size = (MULTIPLIER * (size + STRUCT_SIZE)) - STRUCT_SIZE;
	curBreak->available = 0;
	curBreak->next = NULL;
	curBreak->prev = lastVisitedPtr;
	lastVisitedPtr->next = curBreak;

	if(curBreak->size > size)
		splitChunk(curBreak, size);

	return curBreak;  
}


/* mergeChunkPrev: merge one freed chunk with its predecessor (in case it is free as well)
 *      chunkStatus* freed: pointer to the block of memory to be freed.
 *           retval: void, the function modifies the list
 *           */
void mergeChunkPrev(chunkStatus *freed)
{ 
	chunkStatus *prev;
	prev = freed->prev;

	if(prev != NULL && prev->available == 1)
	{
		prev->size = prev->size + freed->size + STRUCT_SIZE;
		prev->next = freed->next;
		if( (freed->next) != NULL )
			(freed->next)->prev = prev;
	}
}

/* mergeChunkNext: merge one freed chunk with the following chunk (in case it is free as well)
 *      chunkStatus* freed: pointer to the block of memory to be freed.
 *           retval: void, the function modifies the list
 *           */
void mergeChunkNext(chunkStatus *freed)
{  
	chunkStatus *next;
	next = freed->next;

	if(next != NULL && next->available == 1)
	{
		freed->size = freed->size + STRUCT_SIZE + next->size;
		freed->next = next->next;
		if( (next->next) != NULL )
			(next->next)->prev = freed;
	}
}


/* printList: print the entire liked list. For debug purposes
 *      chunkStatus* headptr: points to the begin of the list
 *           retval: void, just print
 *           */
void printList(chunkStatus *headptr)
{
	int i = 0;
	chunkStatus *p = headptr;

	while(p != NULL)
	{
		printf("[%d] p: %d\n", i, p);
		printf("[%d] p->size: %d\n", i, p->size);
		printf("[%d] p->available: %d\n", i, p->available);
		printf("[%d] p->prev: %d\n", i, p->prev);
		printf("[%d] p->next: %d\n", i, p->next);
		printf("__________________________________________________\n");
		i++;
		p = p->next;
	}
}

/* mymalloc: allocates memory on the heap of the requested size. The block
 *              of memory returned should always be padded so that it begins
 *                           and ends on a word boundary.
 *                                unsigned int size: the number of bytes to allocate.
 *                                     retval: a pointer to the block of memory allocated or NULL if the 
 *                                                  memory could not be allocated. 
 *                                                               (NOTE: the system also sets errno, but we are not the system, 
 *                                                                                   so you are not required to do so.)
 *                                                                                   */
void *mymalloc(unsigned int _size) 
{


	void *brkPoint1 = NULL;
	unsigned int size = ALIGN(_size);
	int memoryNeed = MULTIPLIER * (size + STRUCT_SIZE);
	chunkStatus *ptr = NULL;
	chunkStatus *newChunk = NULL;

	pthread_mutex_lock(&lock);
	brkPoint0 = sbrk(0);

	if(head == NULL)				
	{
		if(sbrk(memoryNeed) == (void*) -1)		
		{
			pthread_mutex_unlock(&lock);
			return NULL;
		}

		brkPoint1 = sbrk(0);
		head = brkPoint0;
		head->size = memoryNeed - STRUCT_SIZE;
		head->available = 0;
		head->next = NULL;
		head->prev = NULL;

		ptr = head;


		if(MULTIPLIER > 1)  
			splitChunk(ptr, size);

		pthread_mutex_unlock(&lock);

		return ptr->end;
	}

	else								
	{
		chunkStatus *freeChunk = NULL;
		freeChunk = findChunk(head, size);

		if(freeChunk == NULL)					
		{
			freeChunk = increaseAllocation(lastVisited, size);	
			if(freeChunk == NULL) 					
			{
				pthread_mutex_unlock(&lock);
				return NULL;
			}
			pthread_mutex_unlock(&lock);
			return freeChunk->end;
		}

		else						
		{
			if(freeChunk->size > size)			
				splitChunk(freeChunk, size);
		}    
		pthread_mutex_unlock(&lock);    
		return freeChunk->end;
	}  
}

/* myfree: unallocates memory that has been allocated with mymalloc.
 *      void *ptr: pointer to the first byte of a block of memory allocated by 
 *                      mymalloc.
 *                           retval: 0 if the memory was successfully freed and 1 otherwise.
 *                                        (NOTE: the system version of free returns no error.)
 *                                        */
unsigned int myfree(void *ptr) {

	/*free(ptr);
	 * 	return 0;
	 * 		#endif	
	 * 			return 1; */
	pthread_mutex_lock(&lock);

	chunkStatus *toFree;
	toFree = ptr - STRUCT_SIZE;

	if(toFree >= head && toFree <= (chunkStatus *)brkPoint0)
	{
		toFree->available = 1;	
		mergeChunkNext(toFree);
		mergeChunkPrev(toFree);
		pthread_mutex_unlock(&lock);
		return 0;

	}
	else
	{

		pthread_mutex_unlock(&lock);
		return 1;
	}
}
