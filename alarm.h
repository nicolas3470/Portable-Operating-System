#ifndef __ALARM_H_
#define __ALARM_H_

#include "queue.h"
#include "machineprimitives.h"

/*
 * This is the alarm interface. You should implement the functions for these
 * prototypes, though you may have to modify some other files to do so.
 */

typedef struct alarm{
	int id;
	long end_tick;
	proc_t func;
	arg_t arg;
}* alarm_t;

extern queue_t alarm_queue;

/* register an alarm to go off in "delay" milliseconds, call func(arg) */
int register_alarm(int delay, void (*func)(void*), void *arg);

void deregister_alarm(int alarmid);

#endif
