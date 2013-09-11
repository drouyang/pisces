/* Pisces Memory Management
 * (c) 2013, Jiannan Ouyang, (ouyang@cs.pitt.edu)
 * (c) 2013, Jack Lange, (jacklange@cs.pitt.edu)
 */

#ifndef _PISCES_LOADER_H_
#define _PISCES_LOADER_H_


struct pisces_enclave;

struct launch_code_header {
    u64 kernel_addr;
    u64 real_mode_data_addr;
} __attribute__((packed));


int kick_offline_cpu(struct pisces_enclave * enclave);

int launch_enclave(struct pisces_enclave * enclave);
#endif
