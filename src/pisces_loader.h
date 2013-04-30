/* Pisces Memory Management
 * (c) 2013, Jiannan Ouyang, (ouyang@cs.pitt.edu)
 * (c) 2013, Jack Lange, (jacklange@cs.pitt.edu)
 */

#ifndef _PISCES_LOADER_H_
#define _PISCES_LOADER_H_

#include "pisces.h"
#include "enclave.h"




int kick_offline_cpu(struct pisces_enclave * enclave);

int launch_enclave(struct pisces_enclave * enclave);
#endif
