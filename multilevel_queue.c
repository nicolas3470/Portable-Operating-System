/*
 * Multilevel queue manipulation functions  
 */
#include "multilevel_queue.h"
#include <stdlib.h>
#include <stdio.h>

/*
 * Returns an empty multilevel queue with number_of_levels levels. On error should return NULL.
 */
multilevel_queue_t multilevel_queue_new(int number_of_levels)
{
	int i;
	multilevel_queue_t mqueue; 
	if(number_of_levels == 0) return NULL;
	mqueue = (multilevel_queue_t) malloc(sizeof(struct multilevel_queue));
	if(mqueue==NULL) return NULL;
	mqueue->num_levels = number_of_levels;
	for(i=0;i<number_of_levels;i++){
		mqueue->queues[i] = queue_new();
	}

	return mqueue;
}

/*
 * Appends an any_t to the multilevel queue at the specified level. Return 0 (success) or -1 (failure).
 */
int multilevel_queue_enqueue(multilevel_queue_t queue, int level, any_t item)
{
	if(queue==NULL || (queue->num_levels-1) < level) 
		return -1;
	return queue_append(queue->queues[level],item);
}

/*
 * Dequeue and return the first any_t from the multilevel queue starting at the specified level. 
 * Levels wrap around so as long as there is something in the multilevel queue an item should be returned.
 * Return the level that the item was located on and that item if the multilevel queue is nonempty,
 * or -1 (failure) and NULL if queue is empty.
 */
int multilevel_queue_dequeue(multilevel_queue_t queue, int level, any_t *item)
{
	if(queue==NULL || (queue->num_levels-1) < level) 
		return -1;
	return queue_dequeue(queue->queues[level],item);
}

/* 
 * Free the queue and return 0 (success) or -1 (failure). Do not free the queue nodes; this is
 * the responsibility of the programmer.
 */
int multilevel_queue_free(multilevel_queue_t queue)
{
	int i;
	if(queue==NULL) return -1;

	for(i=0;i<queue->num_levels;i++){
		queue_free(queue->queues[i]);
	}
	free(queue);

	return 0;
}

void multilevel_queue_print_minithreads(multilevel_queue_t queue){
	int i;
	printf("\n");
	for(i=0;i<queue->num_levels;i++){
		printf("L%d: ",i);
		queue_print_minithreads(queue->queues[i]);
	}
	printf("\n\n");
}
