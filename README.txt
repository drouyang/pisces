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

