/*
 * Generic queue manipulation functions  
 */
#ifndef __QUEUE_H__
#define __QUEUE_H__

/*
 * any_t is a pointer.  This allows you to put arbitrary structures on
 * the queue.
 */
typedef void *any_t;

/*
 * PFany is a pointer to a function that can take two any_t arguments
 * and return an integer.
 */
typedef int (*PFany)(any_t, any_t);

/*
 * queue_t is a pointer to an internally maintained data structure.
 * Clients of this package do not need to know how queues are
 * represented.  They see and manipulate only queue_t's. 
 */
typedef struct queue_node* queue_node_t;
typedef struct queue* queue_t;
struct queue_node{
	queue_node_t prev;
	queue_node_t next;
	any_t data;
};

struct queue{
	queue_node_t head;
	queue_node_t tail;
};

/*
 * Return an empty queue. On error should return NULL.
 */
extern queue_t queue_new();

/*
 * Prepend an any_t to a queue (both specifed as parameters).  Return
 * 0 (success) or -1 (failure).
 */
extern int queue_prepend(queue_t, any_t);

/*
 * Appends an any_t to a queue (both specifed as parameters).  Return
 * 0 (success) or -1 (failure).
 */
extern int queue_append(queue_t, any_t);

/*
 * Dequeue and return the first any_t from the queue. Return 0
 * (success) and first item if queue is nonempty, or -1 (failure) and
 * NULL if queue is empty.
 */
extern int queue_dequeue(queue_t, any_t *);

/*
 * Iterate the function parameter over each element in the queue.  The
 * additional any_t argument is passed to the function as its first
 * argument and the queue element is the second.  
 * Return 0 (success) or -1 (failure).
 */
extern int queue_iterate(queue_t, PFany, any_t);

/* 
 * Free the queue and return 0 (success) or -1 (failure).
 */
extern int queue_free (queue_t);

/*
 * Return the number of items in the queue.
 */
extern int queue_length(queue_t queue);

/* 
 * Delete the specified item from the given queue. 
 * Return -1 on error.
 */
extern int queue_delete(queue_t queue, any_t* item);
extern int queue_delete_cond(queue_t queue, PFany f, any_t fstparam);


extern void queue_print_minithreads(queue_t queue);
extern void queue_print_alarms(queue_t queue);

#endif __QUEUE_H__
