#ifndef _PISCES_MOD_H_
#define _PISCES_MOD_H_


extern unsigned long mem_base;

extern unsigned long mem_len;

extern unsigned long cpu_id;

extern char *kernel_path;

extern char *initrd_path;

extern char *boot_cmd_line;

extern unsigned long mpf_found_addr;

extern struct shared_info_t *shared_info;

#endif
