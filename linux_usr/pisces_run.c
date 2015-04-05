#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <ctype.h>


#include "pisces_ctrl.h"
#include "pisces.h"

#define DEFAULT_NUM_RANKS       1
#define DEFAULT_CPU_LIST        NULL
#define DEFAULT_USE_LARGE_PAGES 0
#define DEFAULT_USE_SMARTMAP    0
#define DEFAULT_HEAP_SIZE       (16 * 1024 * 1024)
#define DEFAULT_STACK_SIZE      (256 * 1024)
#define DEFAULT_ENVP            ""

unsigned int         num_ranks       = DEFAULT_NUM_RANKS;
char               * cpu_list        = DEFAULT_CPU_LIST;
unsigned char        use_large_pages = DEFAULT_USE_LARGE_PAGES;
unsigned char        use_smartmap    = DEFAULT_USE_SMARTMAP;
unsigned long long   heap_size       = DEFAULT_HEAP_SIZE;
unsigned long long   stack_size      = DEFAULT_STACK_SIZE;
char               * name            = NULL;
char               * envp            = DEFAULT_ENVP;
char               * exe_path        = NULL;
char               * exe_argv        = NULL;

int cmd_line_np          = 0;
int cmd_line_cpu_list    = 0;
int cmd_line_large_pages = 0;
int cmd_line_smartmap    = 0;
int cmd_line_heap_size   = 0;
int cmd_line_stack_size  = 0;
int cmd_line_name        = 0;
int cmd_line_envp        = 0;
int cmd_line_exe         = 1;


static void usage() {
    printf("pisces_run: Job Launch utility for pisces\n\n"		\
	   " Launches a job as specified in command line options or in a job_file.\n\n"						\
	   "Usage: pisces_run <enclave_dev> [options] <-f job_file | exe args...>\n" \
	   " Options: \n"						           \
	   "\t[-np <ranks>]               (default: 1)        : Number of ranks  \n" \
	   "\t[--cpulist=<cpus>]          (default: 0,1,2...) : comma separated list of target CPUs \n"  \
	   "\t[--use_large_pages]         (default: n)        : Use large pages  \n"		\
	   "\t[--use_smartmap]            (default: n)        : Use smartmap     \n"	                   \
	   "\t[--heap_size=<size in MB>]  (default: 16MB)     : Heap size in MB  \n"            	   \
	   "\t[--stack_size=<size in MB>] (default: 256KB)    : Stack size in MB \n" 		           \
	   "\t[--name=<name>]             (default: exe name) : Name of Job      \n"		           \
	   "\t[--envp=<envp>]             (default: NULL)     : ENVP string      \n"		           \
	   );
    
    exit(-1);
}


int main(int argc, char ** argv) {
    int       pisces_id    = -1;
    int       use_job_file =  0;
    uint64_t  cpu_mask     =  0;

    /* Parse Options */
    {
	int  opt_index = 0;
	char c         = 0;
	
	opterr = 1;

	static struct option long_options[] = {
	    {"np",               required_argument, &cmd_line_np,          1},
	    {"cpulist",          required_argument, &cmd_line_cpu_list,    1},
	    {"use-large-pages",  no_argument,       &cmd_line_large_pages, 1},
	    {"use-smartmap",     no_argument,       &cmd_line_smartmap,    1},
	    {"heap_size",        required_argument, &cmd_line_heap_size,   1},
	    {"stack_size",       required_argument, &cmd_line_stack_size,  1},
	    {"name",             required_argument, &cmd_line_name,        1},
	    {"envp",             required_argument, &cmd_line_envp,        1},
	    {0, 0, 0, 0}
	};


	while ((c = getopt_long_only(argc, argv, "f:", long_options, &opt_index)) != -1) {
	    switch (c) {
		case 'f': 
		    printf("JOBFILE\n");
		    use_job_file = 1;
		    cmd_line_exe = 0;
		    break;
		case 0:
		    printf("longoptindex=%d required_argument=%d\n", opt_index, required_argument);
		    printf("option=%s arg=%s\n", long_options[opt_index].name, optarg);
		    

		    switch (opt_index) {
			case 0: {

			    if (!isdigit(*optarg)) {
				usage();
			    }
			 
			    num_ranks = strtol(optarg, NULL, 10);
			    
			    if (num_ranks == 0) {
				// Invalid Setting _OR_ error from a corrupt string
				usage();
			    }
			    break;
			}
			case 1: {
			    cpu_list = optarg;
			    break;
			}
			case 2: {
			    use_large_pages = 1;
			    break;
			}
			case 3: {
			    use_smartmap = 1;
			    break;
			}
			case 4: {
			    unsigned long long heap_size_in_MB = 0;

			    if (!isdigit(*optarg)) {
				usage();
			    }
			 
			    heap_size_in_MB = strtol(optarg, NULL, 10);
			    
			    if (heap_size_in_MB == 0) {
				// Invalid Setting _OR_ error from a corrupt string
				usage();
			    }

			    heap_size = heap_size_in_MB * 1024 * 1024;
		    
			    break;
			}
			case 5: {
			    unsigned long long stack_size_in_MB = 0;

			    if (!isdigit(*optarg)) {
				usage();
			    }
			 
			    stack_size_in_MB = strtol(optarg, NULL, 10);
			    
			    if (stack_size_in_MB == 0) {
				// Invalid Setting _OR_ error from a corrupt string
				usage();
			    }

			    stack_size = stack_size_in_MB * 1024 * 1024;
		    
			    break;
			}
			case 6: {
			    name = optarg;
			    break;
			} 
			case 7: {
			    envp = optarg;
			    break;
			}
			default:
			    break;

		    };
		    
		    break;
		case '?':
		default:
		    printf("Error parsing command line (%c)\n", c);
		    usage();
	    }

	}
	
	printf("argc=%d, optind=%d\n", argc, optind);


	/*  At this point we have <enclave> <exe_path> <exe_args> left 
	 *  OR if a job file is set then just <enclave> left 
	 */

	if (use_job_file) {
	    printf("Error Job files not supported yet\n");
	    exit(0);
	    
	    if (optind + 1 != argc) {
		usage();
	    }
	    
	    pisces_id = get_pisces_id_from_path(argv[optind]);
	    
	} else {
	    int i = 0;

	    pisces_id = get_pisces_id_from_path(argv[optind]);
	    exe_path  = argv[optind + 1];
	    
	    for (i = 0; i < argc - (optind + 2); i++) {

		if (exe_argv) {
		    char * tmp_argv = NULL;

		    asprintf(&tmp_argv, "%s %s", exe_argv, argv[optind + i + 2]);

		    free(exe_argv);
		
		    exe_argv = tmp_argv;
		} else {
		    exe_argv = argv[optind + i + 2];
		}
	    }
	    	    
	}

    }


    if (name == NULL) {
	name = strrchr(exe_path, '/');

	if (name == NULL) {
	    name = exe_path;
	} else {
	    /* Skip past the '/' */
	    name++;
	}	
    }



    if (cpu_list == NULL) {
	cpu_mask = 0xffffffffffffffffULL;
    } else {
	char * iter_str = NULL;
	
	while ((iter_str = strsep(&cpu_list, ","))) {

	    int idx = atoi(iter_str);
	    
	    if ((idx == 0) && (iter_str[0] != '0')) {
		printf("Error: Invalid CPU entry (%s)\n", iter_str);
		return -1;
	    }

	    cpu_mask |= (0x1ULL << idx);
	}
    }

    

    return pisces_run_job(pisces_id, 
			  name, 
			  exe_path, 
			  exe_argv, 
			  envp, 
			  use_large_pages, 
			  use_smartmap, 
			  num_ranks, 
			  cpu_mask,
			  heap_size, 
			  stack_size);
}
