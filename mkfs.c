/* 
 * Spawn three threads and get them to sleep.
*/

#include "minithread.h"
#include "disk.h"

#include <stdio.h>
#include <stdlib.h>


int mkfs_thread(arg_t arg) {
	printf("mkfs_thread starts.\n");
	printf("... nothing happens...\n");
	printf("mkfs_thread finishes\n");
	return 0;
}

void main( int argc, char *argv[] ) {
	char str[256];
	printf ("Enter a disk size (# of blocks): ");
	gets (str);

	disk_size = atoi (str);
	use_existing_disk = 0;
	disk_name = "disk0";
	disk_flags = DISK_READWRITE;
	
	minithread_system_initialize((proc_t)mkfs_thread, (arg_t)NULL);
}
