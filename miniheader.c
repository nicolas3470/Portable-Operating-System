#include "miniheader.h"

void pack_unsigned_int(char* buf, unsigned int val)
{
	unsigned char* ubuf = buf;
	ubuf[0] = (val>>24) & 0xff;
	ubuf[1] = (val>>16) & 0xff;
	ubuf[2] = (val>>8) & 0xff;
	ubuf[3] = val & 0xff;
}

unsigned int unpack_unsigned_int(char *buf)
{
	unsigned char* ubuf = buf;
	return (unsigned int) (ubuf[0]<<24) | (ubuf[1]<<16) | (ubuf[2]<<8) | ubuf[3];
}

void pack_unsigned_short(char* buf, unsigned short val)
{
	unsigned char* ubuf = buf;
	ubuf[0] = (val>>8) & 0xff;
	ubuf[1] = val & 0xff;
}

unsigned short unpack_unsigned_short(char* buf)
{
	unsigned char* ubuf = buf;
	return (unsigned short) (ubuf[0]<<8) | ubuf[1];
}

void pack_address(char* buf, network_address_t address)
{
	unsigned int* addr_ptr = (unsigned int*) address;
	pack_unsigned_int(buf, addr_ptr[0]);
	pack_unsigned_int(buf+sizeof(unsigned int), addr_ptr[1]);
}

void unpack_address(char* buf, network_address_t address)
{
	unsigned int* addr_ptr = (unsigned int*) address;
	addr_ptr[0] = unpack_unsigned_int(buf);
	addr_ptr[1] = unpack_unsigned_int(buf+sizeof(unsigned int));
}

void pack_mini_header(mini_header_t header,
	char protocol, 
	network_address_t source_addr, 
	unsigned short source_port,
	network_address_t dest_addr,
	unsigned short dest_port){
		header->protocol = protocol;
		pack_address(header->source_address,source_addr);
		pack_address(header->destination_address,dest_addr);
		pack_unsigned_short(header->source_port,source_port);
		pack_unsigned_short(header->destination_port,dest_port);
}

void unpack_mini_header(mini_header_t header,
	char protocol, 
	network_address_t source_addr, 
	unsigned short source_port,
	network_address_t dest_addr,
	unsigned short dest_port){
		protocol = header->protocol;
		unpack_address(header->source_address,source_addr);
		unpack_address(header->destination_address,dest_addr);
		source_port = unpack_unsigned_short(header->source_port);
		dest_port = unpack_unsigned_short(header->destination_port);
}

void pack_mini_header_reliable(mini_header_reliable_t header,
	char protocol, 
	network_address_t source_addr, 
	unsigned short source_port,
	network_address_t dest_addr,
	unsigned short dest_port,
	char msg_type,
	unsigned int seq,
	unsigned int ack){
		header->protocol = protocol;
		pack_address(header->source_address,source_addr);
		pack_unsigned_short(header->source_port,source_port);
		pack_address(header->destination_address,dest_addr);
		pack_unsigned_short(header->destination_port,dest_port);
		header->message_type = msg_type;
		pack_unsigned_int(header->seq_number,seq);
		pack_unsigned_int(header->ack_number,ack);
}

void unpack_mini_header_reliable(mini_header_reliable_t header,
	char protocol, 
	network_address_t source_addr, 
	unsigned short source_port,
	network_address_t dest_addr,
	unsigned short dest_port,
	char msg_type,
	unsigned int seq,
	unsigned int ack){
		protocol = header->protocol;
		unpack_address(header->source_address,source_addr);
		source_port = unpack_unsigned_short(header->source_port);
		unpack_address(header->destination_address,dest_addr);
		dest_port = unpack_unsigned_short(header->destination_port);
		msg_type = header->message_type;
		seq = unpack_unsigned_int(header->seq_number);
		ack = unpack_unsigned_int(header->ack_number);

}