/* 
 * V3 Control utility
 * (c) Jack lange, 2010
 *
 * Modified by Jiannan Ouyang, 2013
 */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <string.h>
#include <dirent.h> 


#define SYS_PATH "/sys/devices/system/memory/"

#define BUF_SIZE 128

#define OFFLINE 1
#define ONLINE 0

unsigned int block_size_bytes = 0;

int dir_filter(const struct dirent * dir) {
    if (strncmp("memory", dir->d_name, 6) == 0) {
	return 1;
    }

    return 0;
}



int dir_cmp(const struct dirent ** dir1, const struct dirent ** dir2) {
    int num1 = atoi((*dir1)->d_name + 6);
    int num2 = atoi((*dir2)->d_name + 6);

    return num1 - num2;
}


int offline_block(int index) {
    FILE * block_file = NULL;
    char fname[256];
    
    memset(fname, 0, 256);
    
    snprintf(fname, 256, "%smemory%d/state", SYS_PATH, index);
    
    block_file = fopen(fname, "r+");
    
    if (block_file == NULL) {
	printf("Could not open block file %d\n", index);
	perror("\tError:");
	return -1;
    }
    
    
    printf("Offlining block %d (0x%p)\n", index, index * block_size_bytes);
    fprintf(block_file, "offline\n");
    
    fclose(block_file);

    return 0;
}


int get_block_status(int index) {
    char fname[BUF_SIZE];
    char status_buf[BUF_SIZE];
    int block_fd;


    memset(fname, 0, BUF_SIZE);
    memset(status_buf, 0, BUF_SIZE);

    snprintf(fname, BUF_SIZE, "%smemory%d/state", SYS_PATH, index);

		
    block_fd = open(fname, O_RDONLY);
		
    if (block_fd == -1) {
	printf("Could not open block file %d\n", index);
	perror("\tError:");
	return -1;
    }
		
    if (read(block_fd, status_buf, BUF_SIZE) <= 0) {
	perror("Could not read block status");
	return -1;
    }

    //printf("Checking block status %d (%s)...", index, fname);

    if (strncmp(status_buf, "offline", strlen("offline")) == 0) {
	//printf("OFFLINE\n");
	return OFFLINE;
    } else if (strncmp(status_buf, "online", strlen("online")) == 0) {
	//printf("ONLINE\n");
	return ONLINE;
    } 

    // otherwise we have an error

    //printf("ERROR\n");
    return -1;
}


/*
int add_palacios_memory(unsigned long long base_addr, unsigned long num_pages) {
    int v3_fd = 0;
    struct v3_mem_region mem;

    printf("Giving Palacios %lluMB of memory at (%p) \n", 
	   (num_pages * 4096) / (1024 * 1024), base_addr);
    
    mem.base_addr = base_addr;
    mem.num_pages = num_pages;

    v3_fd = open(v3_dev, O_RDONLY);

    if (v3_fd == -1) {
	printf("Error opening V3Vee control device\n");
	return -1;
    }

    ioctl(v3_fd, V3_ADD_MEMORY, &mem); 

    // Close the file descriptor.
    close(v3_fd);

    return 0;
}
*/



int main(int argc, char * argv[]) {
    int bitmap_entries = 0;
    unsigned char * bitmap = NULL;
    int num_blocks = 0;    
    int reg_start = 0;
    int mem_ready = 0;

    if (argc != 2) {
	printf("usage: gemini_mem <num of memory blocks>\n");
	return -1;
    }


    num_blocks = atoll(argv[1]);

    printf("Trying to find %d blocks of memory\n", num_blocks);

    /* Figure out the block size */
    {
	int tmp_fd = 0;
	char tmp_buf[BUF_SIZE];

	tmp_fd = open(SYS_PATH "block_size_bytes", O_RDONLY);

	if (tmp_fd == -1) {
	    perror("Could not open block size file: " SYS_PATH "block_size_bytes");
	    return -1;
	}
        
	if (read(tmp_fd, tmp_buf, BUF_SIZE) <= 0) {
	    perror("Could not read block size file: " SYS_PATH "block_size_bytes");
	    return -1;
	}
        
	close(tmp_fd);

	block_size_bytes = strtoll(tmp_buf, NULL, 16);

	printf("Memory block size is %dMB (%d bytes)\n", block_size_bytes / (1024 * 1024), block_size_bytes);
    }
    

    

    /* Scan the memory directories */
    {
	struct dirent ** namelist = NULL;
	int size = 0;
	int i = 0;
	int j = 0;
	int last_block = 0;

	last_block = scandir(SYS_PATH, &namelist, dir_filter, dir_cmp);
	bitmap_entries = atoi(namelist[last_block - 1]->d_name + 6) + 1;

	size = bitmap_entries / 8;
	if (bitmap_entries % 8) size++;

	bitmap = malloc(size);

	if (!bitmap) {
            printf("ERROR: could not allocate space for bitmap\n");
            return -1;
	}

	memset(bitmap, 0, size);

        // construct online bitmap
        // printf("Available memory scanning ...\n");
	for (i = 0; j < bitmap_entries - 1; i++) {
	    struct dirent * tmp_dir = namelist[i];
	    int block_fd = 0;	    
	    char status_str[BUF_SIZE];
	    char fname[BUF_SIZE];

	    memset(status_str, 0, BUF_SIZE);
	    memset(fname, 0, BUF_SIZE);

	    snprintf(fname, BUF_SIZE, "%s%s/removable", SYS_PATH, tmp_dir->d_name);

	    j = atoi(tmp_dir->d_name + 6);
	    int major = j / 8;
	    int minor = j % 8;

	    //printf("Checking %s...", fname);

	    block_fd = open(fname, O_RDONLY);
            
	    if (block_fd == -1) {
		printf("Hotpluggable memory not supported...\n");
		continue;
	    }

	    if (read(block_fd, status_str, BUF_SIZE) <= 0) {
		perror("Could not read block status");
		return -1;
	    }

	    close(block_fd);
            
	    if (atoi(status_str) == 1) {
		//printf("Removable\n");
		
		// check if block is already offline
		if (get_block_status(j) == ONLINE) {
		    bitmap[major] |= (0x1 << minor);
		}
	    } else {
		//printf("Not removable\n");
	    }
	}

    }
    
    // offline <num_blocks> of continuous memory blocks
    {
	int i = 0;
	int j = 0;
	int cur_idx = 0;
	int phy_addr = 0;
	
	for (i = 0; i <= bitmap_entries; i++) {
            j = 0;
            while (j < num_blocks) {
                int major = (i+j) / 8;
                int minor = (i+j) % 8;

                if ((bitmap[major] & (0x1 << minor)) != 0) 
                    j++;
                else 
                    break;
            };
            if (j == num_blocks)
                break;
        }
        
        phy_addr = i * block_size_bytes;
        if ( i <= bitmap_entries) {
            printf("Find %d continious memory blocks start at block %d\n", num_blocks, i);
            for (; i < bitmap_entries; i++) {
                
                int major = i / 8;
                int minor = i % 8;

                if ((bitmap[major] & (0x1 << minor)) != 0) {

                    if (offline_block(i) == -1) {
                        printf("Error: offlined failed on block %d", j);
                        break;
                    }

                    /*  We asked to offline set of blocks, but Linux could have lied. 
                     *  To be safe, check whether blocks were offlined and start again if not 
                     */
                    if (get_block_status(i) == OFFLINE) {
                        cur_idx++;
                    }
                }
                if (cur_idx >= num_blocks) {
                    printf("Offlined %lluMB of memory at (0x%p) \n", block_size_bytes / (1024 * 1024) * num_blocks, phy_addr);
                    break;
                }
            }

        } else {
            printf("Failed to find %d continious memory blocks\n", num_blocks);
        }
    }


    free(bitmap);


    return 0; 
} 
