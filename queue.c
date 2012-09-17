/*
 * Generic queue implementation.
 *
 */
#include "queue.h"
#include "minithread.h"
#include "alarm.h"
#include <stdlib.h>
#include <stdio.h>

/*
 * Return an empty queue.
 */
queue_t queue_new()
{
	queue_t queue = (queue_t) malloc(sizeof(struct queue));
	if(queue == NULL){
		printf("ERROR in queue_new()");
		return NULL;
	}
	queue->head = NULL;
	queue->tail = NULL;
	return queue;
}

/*
 * Prepend an any_t to a queue (both specifed as parameters).  Return
 * 0 (success) or -1 (failure).
 */
int queue_prepend(queue_t queue, any_t item)
{
	queue_node_t node = (queue_node_t) malloc(sizeof(struct queue_node));
	if(node == NULL){
		printf("ERROR in queue_prepend()");
		return -1;
	}
	node->data = item;
	node->prev = NULL;
	if(queue->head==NULL){
		node->next = NULL;
		queue->head = node;
		queue->tail = node;
	}
	else{
		node->next = queue->head;
		queue->head->prev = node;
		queue->head = node;
	}
	return 0;
}

/*
 * Append an any_t to a queue (both specifed as parameters). Return
 * 0 (success) or -1 (failure). 
 */
int queue_append(queue_t queue, any_t item)
{
	queue_node_t node = (queue_node_t) malloc(sizeof(struct queue_node));
	if(node == NULL){
		printf("ERROR in queue_append()");
		return -1;
	}
	node->data = item;
	node->next = NULL;
	if(queue->tail==NULL){
		node->prev = NULL;
		queue->head = node;
		queue->tail = node;
	}
	else{
		node->prev = queue->tail;
		queue->tail->next = node;
		queue->tail = node;
	}
	return 0;
}

/*
 * Dequeue and return the first any_t from the queue or NULL if queue
 * is empty.  Return 0 (success) or -1 (failure).
 */
int queue_dequeue(queue_t queue, any_t* item)
{
	if(queue == NULL || queue->head == NULL){
		*item = NULL;
		return -1;
	}
	*item = queue->head->data;
	if(queue->head==queue->tail)
		queue->tail = NULL;
	queue->head = queue->head->next;
	return 0;
}

/*
 * Iterate the function parameter over each element in the queue.  The
 * additional any_t argument is passed to the function as its first
 * argument and the queue element is the second.  Return 0 (success)
 * or -1 (failure).
 */
int queue_iterate(queue_t queue, PFany f, any_t item)
{
	queue_node_t current_node;

	if(queue == NULL) return -1;
	current_node = queue->head;
	while(current_node != NULL){
		f(item,current_node->data);
		current_node = current_node->next;
	}

	return 0;
}

/*
 * Free the queue and return 0 (success) or -1 (failure).
 */
int queue_free (queue_t queue)
{
	free(queue);

	return 0;
}

/*
 * Return the number of items in the queue.
 */
int queue_length(queue_t queue)
{
	queue_node_t current_node;
	int count = 0;

	current_node = queue->head;
	while(current_node != NULL){
		++count;
		current_node = current_node->next;
	}

	return count;
}

/* 
 * Delete the specified item from the given queue. 
 * Return -1 on error.
 */
int queue_delete(queue_t queue, any_t* item)
{
	queue_node_t current_node;
	if(queue == NULL) return -1;
	if(*item == NULL) return 0;

	current_node = queue->head;
	while(current_node->data != NULL){
		if(&current_node->data == *item){
			if(current_node->prev == NULL && current_node->next == NULL){
				queue->head = NULL;
				queue->tail = NULL;
			}
			else{
				if(current_node->prev == NULL){
					queue->head = current_node->next;
					current_node->next->prev = NULL;
				}
				if(current_node->next == NULL){
					queue->tail = current_node->prev;
					current_node->prev->next = NULL;
				}
				if(current_node->prev != NULL && current_node->next != NULL){
					current_node->prev->next = current_node->next;
					current_node->next->prev = current_node->prev;
				}
			}
			free(current_node);
			return 0;
		}
		current_node = current_node->next;
	}
	return 0;
}

/* 
 * Deletes the first instance of a node that causes f to return 0
 */
int queue_delete_cond(queue_t queue, PFany f, any_t fstparam)
{
	queue_node_t current_node;
	if(queue == NULL) return -1;

	current_node = queue->head;
	while(current_node!=NULL){
		if(f(fstparam,(any_t)current_node)==0){
			if(current_node->prev == NULL && current_node->next == NULL){
				queue->head = NULL;
				queue->tail = NULL;
			}
			else{
				if(current_node->prev == NULL){
					queue->head = current_node->next;
					current_node->next->prev = NULL;
				}
				if(current_node->next == NULL){
					queue->tail = current_node->prev;
					current_node->prev->next = NULL;
				}
				if(current_node->prev != NULL && current_node->next != NULL){
					current_node->prev->next = current_node->next;
					current_node->next->prev = current_node->prev;
				}
			}
			free(current_node);
			return 0;
		}
		current_node = current_node->next;
	}
	return 0;
}

void queue_print_minithreads(queue_t queue){
	queue_node_t current_node = queue->head;
	printf("{");
	while(current_node!=NULL){
		printf(" %d ",((minithread_t)(current_node->data))->id);
		current_node = current_node->next;
	}
	printf("}\n");
}

void queue_print_alarms(queue_t queue){
	queue_node_t current_node = queue->head;
	printf("{");
	while(current_node!=NULL){
		printf(" %d ",((alarm_t)(current_node->data))->end_tick);
		current_node = current_node->next;
	}
	printf("}\n");
}