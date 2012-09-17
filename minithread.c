/*
 * minithread.c:
 *	This file provides a few function headers for the procedures that
 *	you are required to implement for the minithread assignment.
 *
 *	EXCEPT WHERE NOTED YOUR IMPLEMENTATION MUST CONFORM TO THE
 *	NAMING AND TYPING OF THESE PROCEDURES.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include "interrupts.h"
#include "minithread.h"
#include "queue.h"
#include "multilevel_queue.h"
#include "synch.h"
#include "random.h"
#include "alarm.h"
#include "minimsg.h"
#include "network.h"
#include "miniroute.h"
#include "minifile.h"
#include "disk.h"

#include <assert.h>

/********************** IMPORTANT THREADS ***********************/
minithread_t current_thread;
minithread_t idle_thread;
minithread_t reaper_thread;
/****************************************************************/
/************************ THREAD QUEUES *************************/
multilevel_queue_t ready_queue;
queue_t death_queue; 
/****************************************************************/
/************************ MISC VARIABLES ************************/
int max_minithread_id = -1;
tas_lock_t minithread_lock;
/****************************************************************/
/*********************** HELPER FUNCTIONS ***********************/
minithread_t minithread_dequeue(){
	minithread_t dequeued_thread = NULL;

	if(ticks%160 < 80){
		multilevel_queue_dequeue(ready_queue,0,&dequeued_thread);
	}
	else if(ticks%160 < 120){
		multilevel_queue_dequeue(ready_queue,1,&dequeued_thread);
	}
	else if(ticks%160 < 144){
		multilevel_queue_dequeue(ready_queue,2,&dequeued_thread);
	}
	else{
		multilevel_queue_dequeue(ready_queue,3,&dequeued_thread);
	}

	if(dequeued_thread == NULL){
		multilevel_queue_dequeue(ready_queue,0,&dequeued_thread);
	}
	if(dequeued_thread == NULL){
		multilevel_queue_dequeue(ready_queue,1,&dequeued_thread);
	}
	if(dequeued_thread == NULL){
		multilevel_queue_dequeue(ready_queue,2,&dequeued_thread);
	}
	
	if(dequeued_thread == NULL)		return idle_thread;
	else return (dequeued_thread);
}
int destination_level(minithread_t thread){
	if(thread->quanta >= 30) return -1;
	else if(thread->quanta >= 14) return 3;
	else if(thread->quanta >= 5) return 2;
	else if(thread->quanta >= 1) return 1;
	else return 0;
}
int is_critical_tick(){
	return (current_thread->quanta==1 ||
		current_thread->quanta==5 ||
		current_thread->quanta==14 || 
		current_thread->quanta==30);
}
int minithread_enqueue(minithread_t thread){
	int dest_level = destination_level(thread);
	if(thread==idle_thread || thread==reaper_thread) return 0;
	if(dest_level == -1) queue_append(death_queue,(any_t)thread);
	else multilevel_queue_enqueue(ready_queue,dest_level,(any_t)thread);

	return 0;
}
int handle_alarms(){
	queue_node_t current_node = alarm_queue->head;
	queue_node_t next_node;
	while(current_node!=NULL){
		if(((alarm_t) current_node->data)->end_tick==ticks){
			((alarm_t) current_node->data)->
				func(((alarm_t) current_node->data)->arg);
			next_node = current_node->next;
			deregister_alarm(((alarm_t) current_node->data)->id);
		}
		else{
			next_node = current_node->next;
		}
		current_node = next_node;
	}
	return 0;
}
/****************************************************************/
/******************** AUXILLARY THREAD FUNCS ********************/
int final_proc(arg_t final_arg){
	minithread_t dying_thread = current_thread;
	current_thread = minithread_dequeue();
	queue_append(death_queue,(any_t)dying_thread);
	minithread_switch(&dying_thread->stacktop, &current_thread->stacktop);
	return 0;
}
int idle_proc(arg_t idle_arg){
	while(1){
		minithread_yield();
	}
	printf("idle_thread(): exiting idle_thread()");
	return 0;
}
/****************************************************************/


/*
 * A minithread should be defined either in this file or in a private
 * header file.  Minithreads have a stack pointer with to make procedure
 * calls, a stackbase which points to the bottom of the procedure
 * call stack, the ability to be enqueueed and dequeued, and any other state
 * that you feel they must have.
 */


/* minithread functions */

minithread_t minithread_fork(proc_t proc, arg_t arg)
{
	minithread_t thread = minithread_create(proc,arg);
	minithread_enqueue(thread);
	return thread;
}

minithread_t minithread_create(proc_t proc, arg_t arg)
{
	minithread_t thread = (minithread_t) malloc(sizeof(struct minithread));
	if(thread == NULL) return NULL;

	thread->id = (++max_minithread_id);
	thread->quanta = 0;
	minithread_allocate_stack(&thread->stackbase,&thread->stacktop);
	minithread_initialize_stack(&thread->stacktop,proc,arg,(proc_t)final_proc,(arg_t)NULL);

	return thread;
}

minithread_t minithread_self()
{
	return current_thread;
}

int minithread_id()
{
	return current_thread->id;
}

/* DEPRECATED. Beginning from project 2, you should use minithread_unlock_and_stop() instead
 * of this function.
 */
void minithread_stop()
{
	printf("WARNING: minithread_stop() is deprecated.");
}

void minithread_start(minithread_t t)
{
	multilevel_queue_enqueue(ready_queue,0,(any_t)t);
}

void minithread_yield()
{
	minithread_enqueue(current_thread);
	minithread_unlock_and_stop(&minithread_lock);
}

/*
 * This is the clock interrupt handling routine.
 * You have to call minithread_clock_init with this
 * function as parameter in minithread_system_initialize
 */
void clock_handler(void* arg)
{
	++ticks;
	++current_thread->quanta;
	handle_alarms();
	////printf("Tick %d\n",ticks);
	if(is_critical_tick()){
		minithread_yield();
	}
}

void disk_handler(void* data)
{
	int success;
	int blocknum;
	char* buffer;

	disk_interrupt_arg_t* diskinterrupt = (disk_interrupt_arg_t*) data;
	disk_t* disk = diskinterrupt->disk;
	blocknum = diskinterrupt->request.blocknum;
	buffer = diskinterrupt->request.buffer;

	if(disk == NULL){
		diskinterrupt->reply = DISK_REPLY_CRASHED;
		return;
	}
	else if(blocknum > disk_size-1){
		diskinterrupt->reply = DISK_REPLY_ERROR;
		return;
	}
	if(diskinterrupt->request.type == DISK_RESET){
		use_existing_disk = 1; //name,flags,size should be the same
		success = disk_initialize(disk); //re-initialize disk
		if(success == 0) diskinterrupt->reply = DISK_REPLY_OK;
		else diskinterrupt->reply = DISK_REPLY_FAILED;
	}
	else if(diskinterrupt->request.type == DISK_SHUTDOWN){
		success = disk_shutdown(disk);
		if(success == 0) diskinterrupt->reply = DISK_REPLY_OK;
		else diskinterrupt->reply = DISK_REPLY_FAILED;
	}
	else if(diskinterrupt->request.type == DISK_READ){
		strcpy((char*)sblock+(DISK_BLOCK_SIZE //superblock
							+ DISK_BLOCK_SIZE*((int)(0.1*disk_size))  //inodes section
							+ DISK_BLOCK_SIZE*blocknum) //offset in blocks section
			,buffer);
		diskinterrupt->reply = DISK_REPLY_OK;
	}
	else{ //diskinterrupt->request.type == DISK_WRITE
		success = strcmp((char*)sblock+(DISK_BLOCK_SIZE //superblock
							+ DISK_BLOCK_SIZE*((int)(0.1*disk_size))  //inodes section
							+ DISK_BLOCK_SIZE*blocknum) //offset in blocks section
						,buffer);
		if(success == 0) diskinterrupt->reply = DISK_REPLY_OK;
		else diskinterrupt->reply = DISK_REPLY_FAILED;
	}
}

void network_handler(void* data) {
	network_interrupt_arg_t* incoming_packet;
	received_msg_t stored_msg;
	mini_header_t incoming_header;
	unsigned short destination_port;
	
	char* minimsg_w_dataheader;
	int minimsg_w_dataheader_len;

	routing_header_t route_header;

	incoming_packet = (network_interrupt_arg_t*) data;
	
	route_header = (routing_header_t) malloc(sizeof(struct routing_header));
	memcpy(route_header,incoming_packet->buffer,sizeof(struct routing_header));
	
	minimsg_w_dataheader_len = incoming_packet->size - sizeof(struct routing_header);
	minimsg_w_dataheader = (char*) malloc(minimsg_w_dataheader_len);
	memcpy(minimsg_w_dataheader,incoming_packet->buffer +sizeof(struct routing_header),incoming_packet->size - sizeof(struct routing_header));

	incoming_header = (mini_header_t) malloc(sizeof(struct mini_header));
	memcpy(incoming_header,minimsg_w_dataheader,sizeof(struct mini_header));

	//loading up the message to be stored in a received_msg_t struct (defined in minimsg.h)
	stored_msg = (received_msg_t) malloc(sizeof(struct received_msg));
	unpack_address(incoming_header->source_address, stored_msg->foreign_address);
	stored_msg->foreign_listening_port = unpack_unsigned_short(incoming_header->source_port);
	
	memcpy(stored_msg->data,minimsg_w_dataheader +sizeof(struct mini_header),incoming_packet->size - sizeof(struct mini_header));
	stored_msg->data[incoming_packet->size - sizeof(struct mini_header)-sizeof(struct routing_header)] = '\0';

	/*printf("network_handler | incoming_packet: %s(%d)\n",incoming_packet,strlen((char*)incoming_packet));
	printf("network_handler | route_header: %s(%d)\n",route_header,strlen((char*)route_header));
	printf("network_handler | minimsg_w_hdr: %s(%d)\n",minimsg_w_dataheader,strlen(minimsg_w_dataheader));
	printf("network_handler | incoming_header: %s(%d)\n",incoming_header,strlen((char*)incoming_header));
	printf("network_handler | stored_msg->data: %s(%d)\n",stored_msg->data,strlen((char*)stored_msg->data));
	printf("network_handler | stored_msg->foreign_address: "); network_printaddr(stored_msg->foreign_address); printf("\n");
	printf("network_handler | stored_msg->foreign_listening_port: %d\n",stored_msg->foreign_listening_port);*/

	if(incoming_header->protocol == PROTOCOL_MINIDATAGRAM){
		destination_port = unpack_unsigned_short(incoming_header->destination_port);
		//printf("network_handler | destination_port: %d\n",destination_port);
		if(miniport_array[destination_port] != 0){ //Miniport has been created
			queue_append(miniport_array[destination_port]->u.unbound.incoming_data,(any_t)stored_msg);
		}
		else{
			printf("network_handler | ERROR - no port registered at #'%d' \n",destination_port);
		}
	}
	else{
		printf("\nnetwork_handler | ERROR - doesn't currently handle reliable datagrams\n");
	}
}

/*
 * Initialization.
 *
 * 	minithread_system_initialize:
 *	 This procedure should be called from your C main procedure
 *	 to turn a single threaded UNIX process into a multithreaded
 *	 program.
 *
 *	 Initialize any private data structures.
 * 	 Create the idle thread.
 *       Fork the thread which should call mainproc(mainarg)
 * 	 Start scheduling.
 *
 */
void minithread_system_initialize(proc_t mainproc, arg_t mainarg)
{
	minithread_t main_thread;
	stack_pointer_t dummystacktop;
	disk_t* disk0;

	//TESTTESTTEST
	int i;
	char test[4][4];
	char* test_path = "/folder1/folder2/file3.txt";
	char* pch = strtok (test_path,"/");

	for(i=0;i<4;i++) printf("test[i] = %s\n",test[i]);
	//for(i=0;i<4;i++) strcpy(test[i],"0");
	strcpy(test[0],"123");strcpy(test[1],"12");strcpy(test[2],"0");strcpy(test[3],"123");
	for(i=0;i<4;i++) printf("test[i] = %s\n",test[i]);
	printf("strcmp(test[0],test[1])==0: %d\n",strcmp(test[0],test[1])==0);
	printf("strcmp(test[0],test[2])==0: %d\n",strcmp(test[0],test[2])==0);
	printf("strcmp(test[0],test[3])==0: %d\n",strcmp(test[0],test[3])==0);
	//printf("sizeof(test_path): %d\n",sizeof(test_path));
	while (pch != NULL)
	{
		printf ("%s\n",pch);
		pch = strtok (NULL, "/");
	}

	idle_thread = minithread_create((proc_t)idle_proc,(arg_t)NULL);
	main_thread = minithread_create(mainproc,mainarg);
	alarm_queue = queue_new();
	minithread_allocate_stack(&dummystacktop,&dummystacktop);
	ready_queue = multilevel_queue_new(4);
	death_queue = queue_new();
	ticks = 0;
	minithread_lock = 0;
	network_initialize((interrupt_handler_t)network_handler);
	minithread_clock_init((interrupt_handler_t)clock_handler);

	//SETTING UP THE SIMULATION DISK FILE
	use_existing_disk = 0;
	disk_name = "disk0";
	disk_flags = DISK_READWRITE;
	disk_size = 1000;

	disk0 = (disk_t*) malloc(sizeof(disk_t));
	disk_initialize(disk0);
	install_disk_handler((interrupt_handler_t)disk_handler);

	minifile_initialize();
	minimsg_initialize();
	miniroute_initialize();

	current_thread = main_thread;
	minithread_switch(&dummystacktop,&main_thread->stacktop);
}

/*
 * minithread_unlock_and_stop(tas_lock_t* lock)
 *	Atomically release the specified test-and-set lock and
 *	block the calling thread.
 */
void minithread_unlock_and_stop(tas_lock_t* lock)
{
	minithread_t stopped_thread;
	minithread_t yieldee;

	atomic_clear(lock); //LOCK
	yieldee = minithread_dequeue();
	stopped_thread = current_thread;
	current_thread = yieldee;
	atomic_test_and_set(&minithread_lock); //UNLOCK

	if(stopped_thread != yieldee)
		minithread_switch(&stopped_thread->stacktop,&yieldee->stacktop);
}

/*
 * sleep with timeout in milliseconds
 */
void reenqueue_helper(minithread_t thread){
	minithread_enqueue(thread);
}
void minithread_sleep_with_timeout(int delay)
{
	register_alarm(delay,reenqueue_helper,current_thread);
	minithread_unlock_and_stop(&minithread_lock);
}