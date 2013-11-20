#ifndef __PISCES_DATA_H__
#define __PISCES_DATA_H__

/*
 * General data transfer functionality
 * (c) Brian Kocoloski, 2013 (briankoco@cs.pitt.edu)
 */

#include "pisces.h"
#include "pisces_cmds.h"

int pisces_send_cmd(struct pisces_enclave * enclave, struct pisces_cmd * cmd);
int pisces_recv_cmd(struct pisces_enclave * enclave, struct pisces_cmd ** cmd_p);

int pisces_send_resp(struct pisces_enclave * enclave, struct pisces_resp * resp);
int pisces_recv_resp(struct pisces_enclave * enclave, struct pisces_resp ** resp_p);


#endif /* __PISCES_DATA_H__ */
