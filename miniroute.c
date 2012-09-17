#include "miniroute.h"
#include "miniheader.h"
#include "network.h"
#include "queue.h"

int max_routing_header_id;
hashtable_path_t hashed_paths[65521];

/* Performs any initialization of the miniroute layer, if required. */
void miniroute_initialize()
{
	max_routing_header_id = -1;
	//printf("SIZE OF STRUCT HASHTABLE_PATH_T: %d",sizeof(struct hashtable_path));
}

/* sends a miniroute packet, automatically discovering the path if necessary. See description in the
 * .h file.
 */
int miniroute_send_pkt(network_address_t dest_address, int hdr_len, char* hdr, int data_len, char* data)
{
	queue_t path;
	int new_data_len;
	char* new_data;
	int i;
	routing_header_t route_header = (routing_header_t) malloc(sizeof(struct routing_header));
	network_address_t addr_path[MAX_ROUTE_LENGTH];

	//path = hashtable_get(hashed_paths,dest_address);
	if(hashtable_get(hashed_paths,dest_address)==NULL){ //no path has been hashed
		route_header->routing_packet_type = ROUTING_ROUTE_DISCOVERY;
		pack_address(route_header->destination,dest_address);
		pack_unsigned_int(route_header->id,++max_routing_header_id);
		pack_unsigned_int(route_header->ttl, MAX_ROUTE_LENGTH);
		pack_unsigned_int(route_header->path_len,0);
		for(i=0;i<MAX_ROUTE_LENGTH;i++){
			pack_address(route_header->path[i],addr_path[i]);
		}
	}
	else{ //A path for this destination has been hashed
		route_header->routing_packet_type = ROUTING_ROUTE_DISCOVERY;
		pack_address(route_header->destination,dest_address);
		pack_unsigned_int(route_header->id,++max_routing_header_id);
		pack_unsigned_int(route_header->ttl, MAX_ROUTE_LENGTH);
		pack_unsigned_int(route_header->path_len,0);
		for(i=0;i<MAX_ROUTE_LENGTH;i++){
			pack_address(route_header->path[i],addr_path[i]);
		}
	}
	
	new_data_len = hdr_len+ strlen(data);
	new_data = (char*) malloc(new_data_len+1);
	//sprintf(new_data,"%s%s\0",hdr,data);
	memcpy(new_data,hdr,sizeof(struct mini_header));
	memcpy(new_data+sizeof(struct mini_header),data,new_data_len-hdr_len);
	
	/*printf("miniroute_send_pkt | data: %s(%d)\n",data,strlen(data));
	printf("miniroute_send_pkt | hdr: %s(%d)\n",hdr,strlen(hdr));
	printf("miniroute_send_pkt | new_data: %s(%d)\n",new_data,strlen(new_data));
	printf("miniroute_send_pkt | routing_header: %s(%d)\n",route_header,strlen((char*)route_header));*/

	return network_send_pkt(dest_address,sizeof(struct routing_header),(char*)route_header,new_data_len,new_data);
}

/* hashes a network_address_t into a 16 bit unsigned int */
unsigned short hash_address(network_address_t address)
{
	unsigned int result = 0;
	int counter;

	for (counter = 0; counter < 3; counter++)
		result ^= ((unsigned short*)address)[counter];

	return result % 65521;
}

int hashtable_put(hashtable_path_t* table, network_address_t address, queue_t path){
	hashtable_path_t path_entry;
	int i;
	int hashed_address = hash_address(address);
	int orig_address = hash_address(address);
	while(hashed_paths[hashed_address] == 0 || !network_address_same(((hashtable_path_t)hashed_paths[hashed_address])->address,address)){
		hashed_address++;
		if(hashed_address==orig_address) return -1;
		if(hashed_address >= 65521) hashed_address = 0;
	}
	path_entry = (hashtable_path_t) malloc(sizeof(struct hashtable_path));
	memcpy(path_entry->path,path,sizeof(queue_t));
	network_address_copy(address,path_entry->address);
	memcpy(hashed_paths[hashed_address],path_entry,sizeof(struct hashtable_path));
	return 0;
}

queue_t hashtable_get(hashtable_path_t* table, network_address_t address){
	hashtable_path_t path_entry;
	int hashed_address = hash_address(address);
	int orig_address = hash_address(address);
	while(hashed_paths[hashed_address] == 0 || !network_address_same(((hashtable_path_t)hashed_paths[hashed_address])->address,address)){
		hashed_address++;
		if(hashed_address==orig_address) return NULL;
		if(hashed_address >= 65521) hashed_address = 0;
	}
	path_entry = (hashtable_path_t) hashed_paths[hashed_address];
	return (queue_t) path_entry->path;
}