/* 
 * Fastmeme memory offline
 * (c) Jack lange, 2013
 * (c) Brian Kocoloski, 2013
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
#define NUMA_PATH "/sys/devices/system/node/"

#define BUF_SIZE 128

#define OFFLINE 1
#define ONLINE 0

int numa_node = -1;

char * dev_filename = NULL;

#include "../src/pisces.h"



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

int block_is_GB_aligned(int index, int block_size_bytes) {
    unsigned long block_addr = index * block_size_bytes;
    return (!(block_addr % (1 << 30)));
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
    
    
    printf("Offlining block %d (%s)\n", index, fname);
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

    if (numa_node == -1) {
        snprintf(fname, BUF_SIZE, "%smemory%d/state", SYS_PATH, index);
    } else {
        snprintf(fname, BUF_SIZE, "%snode%d/memory%d/state", NUMA_PATH, numa_node, index);
    }
        
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

    printf("Checking offlined block %d (%s)...", index, fname);

    if (strncmp(status_buf, "offline", strlen("offline")) == 0) {
        printf("OFFLINE\n");
        return OFFLINE;
    } else if (strncmp(status_buf, "online", strlen("online")) == 0) {
        printf("ONLINE\n");
        return ONLINE;
    } 

    // otherwise we have an error
    printf("ERROR\n");
    return -1;
}

int get_block_removable(int index) {
    int block_fd = 0;       
    char status_str[BUF_SIZE];
    char fname[BUF_SIZE];

    memset(status_str, 0, BUF_SIZE);
    memset(fname, 0, BUF_SIZE);

    snprintf(fname, BUF_SIZE, "%smemory%d/removable", SYS_PATH, index);

    block_fd = open(fname, O_RDONLY);
    
    if (block_fd == -1) {
        printf("Could not open block removable file (%s)\n", fname);
        return -1;
    }

    if (read(block_fd, status_str, BUF_SIZE) <= 0) {
        perror("Could not read block status");
        return -1;
    }

    close(block_fd);
    return atoi(status_str);
}


unsigned long long get_block_size() {
    unsigned long long block_size_bytes = 0;
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
    return block_size_bytes;
}



int add_palacios_memory(unsigned long long base_addr, unsigned long num_pages) {
    struct memory_range mem;

    printf("Giving Palacios %lluMB of memory at (%p) \n", 
       (num_pages * 4096) / (1024 * 1024), base_addr);
    
    mem.base_addr = base_addr;
    mem.pages = num_pages;

    if (dev_filename == NULL) {
	int fast_fd = 0;
	fast_fd = open("/dev/" DEVICE_NAME, O_RDONLY);

	if (fast_fd == -1) {
	    printf("Error opening fastmem control device\n");
	    return -1;
	}
	
	ioctl(fast_fd, PISCES_ADD_MEM, &mem); 
	/* Close the file descriptor.  */ 
	close(fast_fd);
    } else {
	int enclave_fd = 0;
	int ctrl_fd = 0;;

	enclave_fd = open(dev_filename, O_RDONLY);

	if (enclave_fd < 0) {
	    printf("Error opening Enclave fd\n");
	    return -1;
	}

	ctrl_fd = ioctl(enclave_fd, PISCES_ENCLAVE_CTRL_CONNECT, NULL);
	close(enclave_fd);

	if (ctrl_fd < 0) {
	    printf("Error opening control fd\n");
	    return -1;
	}

	ioctl(ctrl_fd, 101, &mem);

	close(ctrl_fd);
    }

    return 0;
}



int main(int argc, char * argv[]) {
    unsigned long long block_size_bytes = 0;
    int bitmap_entries = 0;
    unsigned char * bitmap = NULL;
    int num_blocks = 0;    
    int reg_start = 0;
    int mem_ready = 0;
    int c = 0;
    int explicit = 0;
    char * block_list_str = NULL;


    opterr = 0;

    while ((c = getopt(argc, argv, "n:i:d:")) != -1) {
        switch (c) {
            case 'n':
                numa_node = atoi(optarg);
                break;

            case 'i':
                explicit = 1;
                block_list_str = optarg;
                break;
	    case 'd':
		dev_filename = optarg;
		break;
         }
    }


    if ((!explicit) && (argc - optind + 1 < 2)) {
        printf("usage: \n");
        printf("%s [-d dev_file] [-n node] <num_blocks>\n", *argv);
        printf("%s [-d dev_file] -i <block_list>\n", *argv);
        return -1;
    }



    /* Figure out the block size */
    block_size_bytes = get_block_size();

    if (explicit) {
        // We need to walk this as a list instead.
        int block_id = atoi(block_list_str);

        if (get_block_removable(block_id) != 1) {
            printf("Error: Block %d not removable\n", block_id);
            return -1;
        }

        if (get_block_status(block_id) != ONLINE) {
            printf("Error: Block %d is already offline\n", block_id);
            return -1;
        }

        if (offline_block(block_id) == -1) {
            printf("Error: Could not offline block %d\n", block_id);
            return -1;
        }

        // Double check block was offlined, it can sometimes fail
        if (get_block_status(block_id) != OFFLINE) {
            printf("Error: Failed to offline block %d\n", block_id);
            return -1;
        }
        
        printf("Adding block of memory at %p (block=%d)to Palacios\n", 
               block_size_bytes * block_id, block_id);
        add_palacios_memory(block_size_bytes * block_id, block_size_bytes / 4096LL);
        
        return 0;
    } 

    num_blocks = atoll(argv[optind]);

    printf("Trying to find %d blocks of memory\n", num_blocks);

    /* Scan the memory directories */
    {
        struct dirent ** namelist = NULL;
        int size = 0;
        int i = 0;
        int j = 0;
        int last_block = 0;
        char dir_path[512];
        
        memset(dir_path, 0, 512);

        if (numa_node == -1) {
            snprintf(dir_path, 512, SYS_PATH);
        } else {
            snprintf(dir_path, 512, "%snode%d/", NUMA_PATH, numa_node);
        }
        
        last_block = scandir(dir_path, &namelist, dir_filter, dir_cmp);

        //printf("last_block = %d\n", last_block);

        if (last_block == -1) {
            printf("Error scan directory (%s)\n", dir_path);
            return -1;
        } else if (last_block == 0) {
            printf("Could not find any memory blocks at (%s)\n", dir_path);
            return -1;
        }

        bitmap_entries = atoi(namelist[last_block - 1]->d_name + 6) + 1;

        size = bitmap_entries / 8;
        if (bitmap_entries % 8) size++;

        printf("%d bitmap entries\n", size);

        bitmap = malloc(size);

        if (!bitmap) {
            printf("ERROR: could not allocate space for bitmap\n");
            return -1;
        }

        memset(bitmap, 0, size);

        for (i = 0; i < last_block - 1; i++) {
            struct dirent * tmp_dir = namelist[i];


            j = atoi(tmp_dir->d_name + 6);
            int major = j / 8;
            int minor = j % 8;

            printf("Checking block %d...", j);
                
            if (get_block_removable(j) == 1) {
                printf("Removable\n");
                
                // check if block is already offline
                if (get_block_status(j) == ONLINE) {
                    bitmap[major] |= (0x1 << minor);
                }
            } else {
                printf("Not removable\n");
            }
        }
    }
    
    
    {
        unsigned long long i = 0;
        unsigned long long j = 0;
        int cur_idx = 0;

        /* Mow many contiguous blocks do we need for 1G? */
        int need_contig_blocks = (1 << 30) / block_size_bytes;
        int found_contig_blocks = 0;

        printf("Looking for %d contiguous blocks, aligned on a 1GB region\n", need_contig_blocks);

        /* First, try to allocate on 1G intervals */
        for (i = 0; i <= bitmap_entries; i++) {
            int major = i / 8;
            int minor = i % 8;

            /* 1 GB is overkill */
            if (num_blocks < cur_idx + need_contig_blocks) {
                break;
            }

            if ((bitmap[major] & (0x1 << minor)) != 0) {

                if (block_is_GB_aligned(i, block_size_bytes)) {
                    found_contig_blocks = 1;
                    continue;
                }

                found_contig_blocks++;
                if (found_contig_blocks == need_contig_blocks) {
                    /* We found a contiguous 1GB */
                    /* Offline the blocks and give it to fastmem */
                    for (j = i - need_contig_blocks + 1; j <= i; j++) {
                        if (offline_block(j) == -1) {
                            found_contig_blocks = 0;
                            break;
                        }

                        if (get_block_status(j) != OFFLINE) {
                            found_contig_blocks = 0;
                            break;
                        }
                    }

                    if (found_contig_blocks != 0) {
                        add_palacios_memory(block_size_bytes * (i - need_contig_blocks + 1), 
                                (block_size_bytes / 4096LL) * need_contig_blocks);
                        cur_idx += found_contig_blocks;
                        found_contig_blocks = 0;
                    }
                }

            } else {
                found_contig_blocks = 0;
            }
        }

        
        for (i = 0; i <= bitmap_entries; i++) {
            int major = i / 8;
            int minor = i % 8;

            if (cur_idx >= num_blocks) break;

            if ((bitmap[major] & (0x1 << minor)) != 0) {

                if (offline_block(i) == -1) {
                    continue;
                }

                /*  We asked to offline set of blocks, but Linux could have lied. 
                 *  To be safe, check whether blocks were offlined and start again if not 
                 */
                if (get_block_status(i) == OFFLINE) {
                    add_palacios_memory(block_size_bytes * i, block_size_bytes / 4096LL);
                    cur_idx++;
                }
            }
            

        }

        if (cur_idx < num_blocks) {
            printf("Could only allocate %d (out of %d) blocks\n", 
               cur_idx, num_blocks);
        }
    }

    free(bitmap);
    return 0; 
} 
