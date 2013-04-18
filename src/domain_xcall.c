/*
 * Pisces cross domain call
 * Author: Jianan Ouyang (ouyang@cs.pitt.edu)
 * Date: 04/2013
 */

#include"domain_xcall.h"

/* 
 * Request an irq vector from linux as a shared irq vector across domains
 * This vector number should be available in OSes from all domains
 * A handler is registered for this vector
 */
int domain_xcall_init(void)
{
    return 0;
}

/*
 * Domain cross call is handled like a syscall.
 * Caller apic_id, xcall_id and params are passed through shared memory
 */
int domain_xcall_handler(void)
{
    return 0;

}
