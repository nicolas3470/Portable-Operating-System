#include "minifile.h"
#include "disk.h"
#include "random.h"
#include "queue.h"
/*
 * struct minifile:
 *     This is the structure that keeps the information about 
 *     the opened file like the position of the cursor, etc.
 */
#define TABLE_SIZE 15

char* current_directory;

//FILE, DIRECTORY, NOT DEFINNED
typedef enum {FIL,DIR,ND} inodetype;

struct block{
	int id;
	union{
		//DIRECTORY BLOCK
		struct{
			char dir_entries[TABLE_SIZE][256];
			inode_t inode_ptrs[TABLE_SIZE];
		}directory;
		
		//INDIRECT BLOCK (FULL BLOCK USED FOR BLOCK PTRS)
		struct{
			//perhaps make this size block_size as well
			block_t direct_ptrs[(DISK_BLOCK_SIZE/4) - 1];
			block_t indirect_ptr;
		}indirect_block;
		
		//FILE DATA BLOCK
		char file_data[DISK_BLOCK_SIZE - sizeof(int)];
	};
};

struct inode{
	int id;
	union{
		struct{
			inodetype type;
			int size; //num blocks occupied by data
			int isopen; //1 = open; 0 = not open

			int blocknum; //block in which cursor is located
			int cursor; //byte address in block

			block_t direct_ptrs[TABLE_SIZE];
			block_t indirect_ptr;
		}data;

		char padding[DISK_BLOCK_SIZE - sizeof(int)];
	};
};

struct superblock{
	union{
		struct{
			int magic_number;
			int disk_size;

			inode_t root_inode;

			queue_t inode_queue_FREE;
			queue_t inode_queue_ACTIVE;
			queue_t block_queue_FREE;
			queue_t block_queue_ACTIVE;
		}data;

		char padding[DISK_BLOCK_SIZE];
	};
};

struct minifile{
	inode_t inode;
};

inode_t create_inode(int id){
	int i;
	inode_t an_inode = (inode_t) malloc(DISK_BLOCK_SIZE);
	an_inode->id = id;
	an_inode->data.blocknum = 0;
	an_inode->data.cursor = 0;
	an_inode->data.isopen = 0;
	an_inode->data.type = ND;
	an_inode->data.size = 0;
	for(i=0;i<TABLE_SIZE;i++)
		strcpy((char*)an_inode->data.direct_ptrs[i],"0");
	strcpy((char*)an_inode->data.indirect_ptr,"0");
	return an_inode;
}

block_t create_block(int id){
	block_t a_block = (block_t) malloc(DISK_BLOCK_SIZE);
	a_block->id = id;
	return a_block;
}

/*
 * Returns a file/directory inode located in the current folder if it exists.
 * If not "0" is returned
 */
inode_t get_inode_from_directory_block(inode_t current_dir, char* filedir_name){
	int i; int j;
	char dir_entries[TABLE_SIZE][256];
	block_t indirect_block;
	inode_t output;
	block_t ptrs[TABLE_SIZE];
	
	strcpy((char*)ptrs,(char*)current_dir->data.direct_ptrs);
	//FIRST CHECK THE DIRECT PTRS
	for(i=0;i<TABLE_SIZE;i++){
		strcpy((char*)dir_entries,(char*)ptrs[i]->directory.dir_entries);
		for(j=0;j<TABLE_SIZE;j++){
			if(strcmp(filedir_name,dir_entries[j])==0){
				output = (inode_t) ptrs[i]->directory.inode_ptrs[j];
				return output;
			}
		}
	}
	//NEXT CHECK ALL INDIRECT BLOCKS
	/*indirect_block = current_dir->data.indirect_ptr;
	while(true){
		if(strcmp((char*)indirect_block,"0")==0) return NULL;
		strcpy((char*)ptrs,(char*)indirect_block->indirect_block.direct_ptrs);
		for(i=0;i<((DISK_BLOCK_SIZE/4) - 1);i++){
			strcpy((char*)dir_entries,(char*)ptrs[i]->directory.dir_entries);
			
		}
		indirect_block = ptrs[0] ->indirect_block.direct_ptrs;
	}*/
	return ((inode_t)0);
}

inode_t get_inode(superblock_t sblock, char* full_path){
	int i;
	inode_t current_inode = sblock->data.root_inode;

	char* pch = strtok(full_path,"/");
	while (pch != NULL)
	{
		//printf ("%s\n",pch);
		current_inode = get_inode_from_directory_block(current_inode,pch);
		if(strcmp((char*)current_inode,(char*)((inode_t)0))==0) return ((inode_t)0); //not found
		pch = strtok(NULL, "/");
	}
	return current_inode;
}

/*
 * Attempts to get extract the next free inode and make it active
 * by moving it from the FREE inode queue to the ACTIVE inode queue.
 * @return success - returns extracted node
 *         failure - returns NULL
 */
inode_t activate_free_inode(superblock_t sblock){
	inode_t extracted_inode = (inode_t) malloc(DISK_BLOCK_SIZE);
	int dequeue_result;
	if(queue_length(sblock->data.inode_queue_FREE)<=0){
		printf("ERROR in activate_free_inode(): No more free inodes");
		return ((inode_t)0);
	}
	else{
		dequeue_result = queue_dequeue(sblock->data.inode_queue_FREE,(any_t*) extracted_inode);
		if(dequeue_result!=0){
			printf("ERROR in activate_free_inode(): Dequeue Error");
			return ((inode_t)0);
		}
		return extracted_inode;
	}
}

/*
 * Attempts to get extract the next free block and make it active
 * by moving it from the FREE block queue to the ACTIVE block queue
 */
block_t activate_free_block(superblock_t sblock){
	block_t extracted_block = (block_t) malloc(sizeof(struct block));
	int dequeue_result;
	if(queue_length(sblock->data.block_queue_FREE)<=0){
		printf("ERROR in activate_free_block(): No more free blocks");
		return ((block_t)0);
	}
	else{
		dequeue_result =  queue_dequeue(sblock->data.block_queue_FREE,(any_t*)extracted_block);
		if(dequeue_result!=0){
			printf("ERROR in activate_free_block(): Dequeue Error");
			return ((block_t)0);
		}
		return extracted_block;
	}
}

/*
 * Extends the # of block pointers for an inode by making use
 * of a new block to hold the extra block pointers. 4092 bytes
 * are used by the direct ptrs and 4 reserved for an indirect.
 */
//extra_block_pointers_t add_indirection(inode_t inode){
//	extra_block_pointers_t indirection;
//	block_t new_block = activate_free_block();
//	if(new_block == NULL)
//		printf("ERROR in add_indirection(): Couldn't find a new block.");
//	indirection = (extra_block_pointers_t) new_block;
//	return indirection;
//}

/*
 * Returns a link to the inode if it exists in the current directory
 */
//inode_t get_inode_in_current(char* filename){
//	int i; int j;
//	inode_t match = NULL;
//	int match_found = 0;
//	extra_block_pointers_t extra_ptrs;
//	for(i=0;i<TABLE_SIZE;i++){
//		for(j=0;j<TABLE_SIZE;j++){
//			if(strcmp(current_directory->data.direct_ptrs[i]->dir.dir_entries[j],filename)==0){
//				match = current_directory->data.direct_ptrs[i]->dir.inode_ptrs[j];
//				match_found = 1;
//				break;
//			}
//		}
//		if(match_found == 1) break;
//	}
//	if(!match_found){
//		while(1){
//			extra_ptrs = (extra_block_pointers_t) current_directory->data.indirect_ptr;
//			if(strcmp((char*)extra_ptrs,(char*)(block_t)0)) break;
//			for(i=0;i<(DISK_BLOCK_SIZE-4)/4;i++){
//				for(j=0;j<DISK_BLOCK_SIZE;j++){
//					if(strcmp(current_directory->data.direct_ptrs[i]->dir.dir_entries[j],filename)==0){
//						match = extra_ptrs->direct_ptrs[i]->dir.inode_ptrs[j];
//						match_found = 1;
//						break;
//					}
//				}
//				if(match_found == 1) break;
//			}
//			printf("get_inode_in_current(): in loop");
//		}
//	}
//	return match;
//}

//Call after setting disk.h's 'disk_size'
superblock_t minifile_initialize(superblock_t sblock){
	int i;
	inode_t root_inode;
	inode_t first_inode;
	block_t first_block;

	if(DEBUG) printf("minifile_initialize starting...\n");
	//CREATE SUPERBLOCK
	sblock = (superblock_t) malloc(sizeof(struct superblock));
	sblock->data.disk_size = disk_size;
	sblock->data.magic_number = 19540119;
	
	//SET UP INODES AND BLOCKS
	sblock->data.inode_queue_FREE = queue_new();
	sblock->data.inode_queue_ACTIVE = queue_new();
	sblock->data.block_queue_FREE = queue_new();
	sblock->data.block_queue_ACTIVE = queue_new();

	if(DEBUG) printf("inode nad block queues created. Their lengths are: %d %d %d %d\n",
		queue_length(sblock->data.inode_queue_FREE),queue_length(sblock->data.inode_queue_ACTIVE),
		queue_length(sblock->data.block_queue_FREE),queue_length(sblock->data.block_queue_ACTIVE));

	root_inode = create_inode(0);
	first_inode = create_inode(1);
	queue_append(sblock->data.inode_queue_ACTIVE,(any_t)root_inode);
	queue_append(sblock->data.inode_queue_FREE,(any_t)first_inode);

	for(i=2;i<(0.1*disk_size);i++){
		queue_append(sblock->data.inode_queue_FREE,(any_t)create_inode(i));
	}

	first_block = (block_t) malloc(sizeof(struct block));
	queue_append(sblock->data.block_queue_FREE,(any_t)first_block);
	for(i=1;i<(0.9*disk_size-1);i++){
		queue_append(sblock->data.block_queue_FREE,(any_t)create_block(i));
	}

	if(DEBUG) printf("inode and block queues created. Their lengths are: %d %d %d %d\n",
		queue_length(sblock->data.inode_queue_FREE),queue_length(sblock->data.inode_queue_ACTIVE),
		queue_length(sblock->data.block_queue_FREE),queue_length(sblock->data.block_queue_ACTIVE));

	//FINISH SETTING UP SUPERBLOCK
	sblock->data.root_inode = root_inode;

	//RETURN SUPERBLOCK
	return sblock;
}

superblock_t sblock;

minifile_t minifile_creat(char *filename){
	minifile_t minifl = (minifile_t) malloc(sizeof(struct minifile));
	inode_t new_inode = activate_free_inode(sblock);
	if(strcmp((char*)((block_t)0),(char*)new_inode)==0){
		printf("ERROR in minifile_creat: no free inodes left");
		return ((minifile_t)0);
	}
	else{
		new_inode->data.type = FIL;
		minifl->inode = new_inode;
		return minifl;
	}
}

minifile_t minifile_open(char *filename, char *mode){

	return NULL;
}

int minifile_read(minifile_t file, char *data, int maxlen){
	int i;
	for(i=0;i<TABLE_SIZE;i++){
		strcpy(data+(i*DISK_BLOCK_SIZE),file->inode->data.direct_ptrs[i]->file_data);
	}
}

int minifile_write(minifile_t file, char *data, int len){
	
	return -1;
}

int minifile_close(minifile_t file){
	
	return -1;
}

int minifile_unlink(char *filename){
	
	return -1;
}

int minifile_mkdir(char *dirname){
	inode_t new_inode = activate_free_inode(sblock);
	inode_t curr_dir = get_inode(sblock,current_directory);
	block_t new_block; int i;
	if(strcmp((char*)((block_t)0),(char*)new_inode)==0){
		printf("ERROR in minifile_creat: no free inodes left");
		return -1;
	}
	else{
		new_inode->data.type = DIR;
		for(i=0;i<TABLE_SIZE;i++) if(strcmp((char*)curr_dir->data.direct_ptrs,(char*)((block_t)0))==0) break;
		if(curr_dir->data.blocknum == 0 && curr_dir->data.cursor == 0){
			//need to make a new block allocation
			new_block = activate_free_block(sblock);
			strcpy(new_block->directory.dir_entries[0],dirname);
			curr_dir->data.cursor += 256;
		}
		else if(curr_dir->data.cursor == TABLE_SIZE*256){
			//new block allocation to next direct ptr
			new_block = activate_free_block(sblock);
			strcpy(new_block->directory.dir_entries[i],dirname);
			
			curr_dir->data.cursor = 0;
		}
		else{
			strcpy(curr_dir->data.direct_ptrs[curr_dir->data.blocknum]
				->directory.dir_entries[curr_dir->data.cursor],dirname);
			curr_dir->data.direct_ptrs[curr_dir->data.blocknum]
			->directory.inode_ptrs[curr_dir->data.cursor + TABLE_SIZE*4] = new_inode;
			curr_dir->data.cursor += 256;
		}
	}
}

int minifile_rmdir(char *dirname){
	
	return -1;
}

int minifile_stat(char *path){
	
	return -1;
} 

int minifile_cd(char *path){
	
	return -1;
}

char **minifile_ls(char *path){
	return NULL;
}

char* minifile_pwd(void){
	
	return NULL;
}
