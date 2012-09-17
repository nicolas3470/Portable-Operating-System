#include <stdio.h>

#include "interrupts.h"
#include "alarm.h"
#include "minithread.h"
#include "queue.h"

queue_t alarm_queue;
int max_alarm_id = -1;

/*
 * insert alarm event into the alarm queue
 * returns an "alarm id", which is an integer that identifies the
 * alarm.
 */
int register_alarm(int delay, void (*func)(void*), void *arg)
{
	alarm_t alarm = (alarm_t) malloc(sizeof(struct alarm));
	alarm->id = (++max_alarm_id);
	alarm->end_tick = ticks + (delay/50 + (delay%50)*1);
	printf("end tick %d registered.\n",alarm->end_tick);
	alarm->func = (proc_t) func;
	alarm->arg = (arg_t) arg;

	queue_append(alarm_queue,(any_t)alarm);

	return 0;
}

/*
 * delete a given alarm  
 * it is ok to try to delete an alarm that has already executed.
 */
int deregister_alarm_helper(any_t alarmid_raw, any_t node_raw){
	queue_node_t node = (queue_node_t) node_raw;
	if(((alarm_t)node->data)->id == (int)alarmid_raw) return 0;
	else return -1;
}
void deregister_alarm(int alarmid)
{
	queue_delete_cond(alarm_queue,
					  (PFany)deregister_alarm_helper,
					  (any_t)alarmid);
}
