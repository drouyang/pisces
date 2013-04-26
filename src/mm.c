/* Pisces Memory Management
 * (c) 2013, Jack Lange, (jacklange@cs.pitt.edu)
 * (c) 2013, Jiannan Ouyang, (ouyang@cs.pitt.edu)
 */

#include "mm.h"
#include "buddy.h"
#include "numa.h"

// Pick up the top level pisces proc directory from pisces_dev.c
extern struct proc_dir_entry * pisces_proc_dir;


static struct buddy_memzone ** memzones = NULL;

uintptr_t pisces_alloc_pages_on_node(u64 num_pages, int node_id) {
    uintptr_t addr = 0;

    if (node_id == -1) {
        int cpu_id = get_cpu();
        put_cpu();

        node_id = numa_cpu_to_node(cpu_id);
    } else if (numa_num_nodes() == 1) {
        // Ignore the NUMA zone here
        node_id = 0;
    } else if (node_id >= numa_num_nodes()) {
        // We are a NUMA aware, and requested an invalid node
	printk(KERN_ERR "Requesting memory from an invalid NUMA node. (Node: %d) (%d nodes on system)\n",
              node_id, numa_num_nodes());
        return 0;
    }

    //    printk("Allocating %llu pages (%llu bytes) order=%d\n",
    //     num_pages, num_pages * PAGE_SIZE, get_order(num_pages * PAGE_SIZE) + PAGE_SHIFT);

    addr = buddy_alloc(memzones[node_id], get_order(num_pages * PAGE_SIZE) + PAGE_SHIFT);
    // PrintDebug("Allocated pages at %p\n", (void *)addr);

    if (!addr) {
        //PrintError("Returning from alloc addr=%p, vaddr=%p\n", (void *)addr, __va(addr));
    }

    return addr;
}

uintptr_t pisces_alloc_pages(u64 num_pages) {
    return pisces_alloc_pages_on_node(num_pages, -1);
}



int pisces_mem_init(void) {
    int num_nodes = numa_num_nodes();
    int node_id = 0;
        
    memzones = kmalloc(GFP_KERNEL, sizeof(struct buddy_memzone *) * num_nodes);
    memset(memzones, 0, sizeof(struct buddy_memzone *) * num_nodes);

    for (node_id = 0; node_id < num_nodes; node_id++) {
	struct buddy_memzone * zone = NULL;
            
	printk("Initializing Zone %d\n", node_id);
	zone = buddy_init(get_order(0x40000000) + PAGE_SHIFT, PAGE_SHIFT, node_id, pisces_proc_dir);

	if (zone == NULL) {
	    printk(KERN_ERR "Could not initialization memory management for node %d\n", node_id);
	    return -1;
	}

	printk("Zone initialized, Adding seed region (order=%d)\n", 
               (get_order(0x40000000) + PAGE_SHIFT));

	//      buddy_add_pool(zone, seed_addrs[node_id], (MAX_ORDER - 1) + PAGE_SHIFT);

	memzones[node_id] = zone;
    }


    return 0;
}


int pisces_add_mem(uintptr_t base_addr, u64 num_pages) {
    int node_id = 0;
    int pool_order = 0;

    node_id = numa_addr_to_node(base_addr);
            
    if (node_id == -1) {
	printk(KERN_ERR "Error locating node for addr %p\n", (void *)base_addr);
	return -1;
    }
              
    printk(KERN_DEBUG "Managing %dMB of memory starting at %llu (%lluMB)\n", 
	   (unsigned int)(num_pages * PAGE_SIZE) / (1024 * 1024), 
	   (unsigned long long)base_addr, 
	   (unsigned long long)(base_addr / (1024 * 1024)));
          
          
    //   pool_order = fls(num_pages); 
    pool_order = get_order(num_pages * PAGE_SIZE) + PAGE_SHIFT;
    buddy_add_pool(memzones[node_id], base_addr, pool_order);

    printk(KERN_DEBUG "%p on node %d\n", (void *)base_addr, numa_addr_to_node(base_addr));


    return 0;

}
