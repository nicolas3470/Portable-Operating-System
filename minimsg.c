/*
 *	Implementation of minimsgs and miniports.
 */
#include "minimsg.h"
#include "miniheader.h"
#include "minithread.h"
#include "synch.h"
#include "queue.h"
#include "interrupts.h"
#include "miniroute.h"


miniport_t miniport_array[65536];
int bound_counter;
network_address_t my_address;

int next_bound_port() {
	int initial_port = bound_counter;
	while(miniport_array[bound_counter] != 0) {
		bound_counter++;
		if(bound_counter==initial_port){
			return -1;
		}
		if (bound_counter > 65535) {
			bound_counter = 0;
		}
	}
	return bound_counter;
}

/* performs any required initialization of the minimsg layer.
 */
void minimsg_initialize() {
	bound_counter = 32768;
	network_get_my_address(my_address);
}

/* Creates an unbound port for listening. Multiple requests to create the same
 * unbound port should return the same miniport reference. It is the responsibility
 * of the programmer to make sure he does not destroy unbound miniports while they
 * are still in use by other threads -- this would result in undefined behavior.
 * Unbound ports must range from 0 to 32767. If the programmer specifies a port number
 * outside this range, it is considered an error.
 */
miniport_t miniport_create_unbound(int port_number) {
	miniport_t new_port;
	if(port_number > 32767 || port_number < 0) {
		printf("Unbound Port Number out of Range Error");
		return NULL;
	} 
	else if (miniport_array[port_number] != 0){
		return miniport_array[port_number];
	}
	else{
		new_port = (miniport_t) malloc(sizeof(struct miniport));
		new_port->type = UNBOUND;
		new_port->port_num = port_number;
		new_port->u.unbound.incoming_data = queue_new();
		new_port->u.unbound.datagrams_ready = semaphore_create();
		semaphore_initialize(new_port->u.unbound.datagrams_ready,1);
		miniport_array[port_number] = new_port;
		return new_port;
	}
}

/* Creates a bound port for use in sending packets. The two parameters, addr and
 * remote_unbound_port_number together specify the remote's listening endpoint.
 * This function should assign bound port numbers incrementally between the range
 * 32768 to 65535. Port numbers should not be reused even if they have been destroyed,
 * unless an overflow occurs (ie. going over the 65535 limit) in which case you should
 * wrap around to 32768 again, incrementally assigning port numbers that are not
 * currently in use.
 */
miniport_t miniport_create_bound(network_address_t addr, int remote_unbound_port_number) {
	miniport_t new_port = (miniport_t) malloc(sizeof(struct miniport));
	new_port->type = BOUND;
	new_port->port_num = next_bound_port();
	if(bound_counter==-1){
		bound_counter = 32768;
		return NULL;
	}
	network_address_copy(addr,new_port->u.bound.remote_address);
	new_port->u.bound.remote_unbound_port = remote_unbound_port_number;
	miniport_array[new_port->port_num] = new_port;
	return new_port;
}

/* Destroys a miniport and frees up its resources. If the miniport was in use at
 * the time it was destroyed, subsequent behavior is undefined.
 */
void miniport_destroy(miniport_t miniport) {
	//wait for miniport to finish before destroying
	//free(miniport);
}

/* Sends a message through a locally bound port (the bound port already has an associated
 * receiver address so it is sufficient to just supply the bound port number). In order
 * for the remote system to correctly create a bound port for replies back to the sending
 * system, it needs to know the sender's listening port (specified by local_unbound_port).
 * The msg parameter is a pointer to a data payload that the user wishes to send and does not
 * include a network header; your implementation of minimsg_send must construct the header
 * before calling network_send_pkt(). The return value of this function is the number of
 * data payload bytes sent not inclusive of the header.
 */
int minimsg_send(miniport_t local_unbound_port, miniport_t local_bound_port, minimsg_t msg, int len) {
	//create header and pack fields
	mini_header_t header;
	
	int num_sent;

	if (strlen(msg) + sizeof(struct mini_header) > MAX_NETWORK_PKT_SIZE) {
		printf("minimsg_send | Message too big\n");
		return -1;
	}
	header = (mini_header_t) malloc(sizeof(struct mini_header));
	header->protocol = PROTOCOL_MINIDATAGRAM;
	pack_mini_header(header,
					PROTOCOL_MINIDATAGRAM,
					my_address,
					local_unbound_port->port_num,
					local_bound_port->u.bound.remote_address,
					local_bound_port->u.bound.remote_unbound_port);
	/*printf("\nminimsg_send() msg: %s\n",msg);
	printf("\nminimsg_send() len: %d\n",len);*/

	num_sent = miniroute_send_pkt(local_bound_port->u.bound.remote_address,
							sizeof(struct mini_header),
							(char*) header,
							len,
							(char*) msg);
	/*printf("\nminimsg_send() num_sent(header size): %d\n",sizeof(struct mini_header));
	printf("\nminimsg_send() num_sent(total size): %d\n",num_sent);*/
	return num_sent;
}

/* Receives a message through a locally unbound port. Threads that call this function are
 * blocked until a message arrives. Upon arrival of each message, the function must create
 * a new bound port that targets the sender's address and listening port, so that use of
 * this created bound port results in replying directly back to the sender. It is the
 * responsibility of this function to strip off and parse the header before returning the
 * data payload and data length via the respective msg and len parameter. The return value
 * of this function is the number of data payload bytes received not inclusive of the header.
 */
int minimsg_receive(miniport_t local_unbound_port, miniport_t* new_local_bound_port, minimsg_t msg, int *len) {

	received_msg_t stored_msg;
	char* stored_data;

	while(queue_length(local_unbound_port->u.unbound.incoming_data)<=0){
		//printf("minimsg_receive | ERROR - No awaiting messages... blocking thread.\n");
		minithread_yield();
	}
	queue_dequeue(local_unbound_port->u.unbound.incoming_data,(any_t*)&stored_msg);

	*len = strlen(stored_msg->data);
	stored_data = (char*) malloc(*len+1);
	memcpy(stored_data,stored_msg->data,*len);
	stored_data[*len] = '\0';

	strcpy(msg,stored_data);

	return *len;
}