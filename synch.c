#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "synch.h"
#include "queue.h"
#include "minithread.h"
#include "interrupts.h"
#include "machineprimitives.h"

/*
 *	You must implement the procedures and types defined in this interface.
 */

/*
 * Semaphores.
 */
typedef struct semaphore {
	tas_lock_t lock;
    int count;
	queue_t wait_queue;
}* semaphore_t;


/*
 * semaphore_t semaphore_create()
 *	Allocate a new semaphore.
 */
semaphore_t semaphore_create() {
	semaphore_t sem = (semaphore_t) malloc(sizeof(struct semaphore));
	sem->count = 0;
	sem->wait_queue = queue_new();
	sem->lock = 0;
	return sem;
}

/*
 * semaphore_destroy(semaphore_t sem);
 *	Deallocate a semaphore.
 */
void semaphore_destroy(semaphore_t sem) {
	free(sem);
}

/*
 * semaphore_initialize(semaphore_t sem, int cnt)
 *	initialize the semaphore data structure pointed at by
 *	sem with an initial value cnt.
 */
void semaphore_initialize(semaphore_t sem, int cnt) {
	sem->count = cnt;
}

/*
 * semaphore_P(semaphore_t sem)
 *	P on the sempahore.
 */
void semaphore_P(semaphore_t sem) {
	while(atomic_test_and_set(&sem->lock)==1) minithread_yield();
	sem->count--;
	if(sem->count < 0){
		queue_append(sem->wait_queue,(any_t)minithread_self());
		atomic_clear(&sem->lock);
		minithread_unlock_and_stop(&minithread_lock);
	}
	else{
		atomic_clear(&sem->lock);
	}
}

/*
 * semaphore_V(semaphore_t sem)
 *	V on the sempahore.
 */
void semaphore_V(semaphore_t sem) {
	minithread_t waiting_thread;
	while(atomic_test_and_set(&sem->lock)==1) minithread_yield();
	sem->count++;
	if(sem->count <= 0){
		queue_dequeue(sem->wait_queue,&waiting_thread);
		if(waiting_thread != NULL)
			minithread_start(waiting_thread);
	}
	atomic_clear(&sem->lock);
}