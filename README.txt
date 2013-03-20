= The GEMINI Kernel Module =
== run ==
1. insert/update gemini module
modins/modupdate
modls

2. reserve memory and cpu
cpurm <N>
cpuls
memrm <N>
memls

3. update gemini parameter <mem_base, mem_len, cpu_map>

4. load OS image

5. start

== design ==
* trampoline
offlined cpu uses trampoline code from linux
we modified the init_code to pointer to our gemini_trampoline, such that
linux jumps to gemini_trampoline right after init the cpu to 64 bit mode.
we then load kernel image, initrd and shared_info into offlined memory.  
A 4M identity mapping page table is setup for the guest kernel. Then we jump
to first instruction of the guest kernel binary, the kernel is position independent
code (PIC), thus can execute on any physical memory given a pre-setup ident mapping

* loader memory map
kernel (2M aligned)
2M gap, because kernel clears __bss section after kernel binary
initrd
shared_info

* gemini console
shared memory based console, the shared memory is organized as a circular queue
We implemented a shared memory based console driver in kitten, which pollutes 
the circular queue. In host OS, we move the produced output to a dedicated buffer
area.

* boot params 
available physical memory map
address/size of kernel, initrd and shared_info
SMP table in the future

== utils ==
* module 
modins
modrm
modupdate (rm and insert)
modls

* memory hot remove
memls <N>
memrm <N>
memadd <N>

* cpu hot remove
cpuls
cpurm <N>
cpuadd <N>

