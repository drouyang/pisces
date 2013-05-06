.data
.align 8
.globl launch_code_start
launch_code_start:
    movq %rax, %cr3
    jmp *%rcx
    hlt

.balign 8
.globl launch_code_target_addr
launch_code_target_addr:    .space 8

.globl launch_code_esi
launch_code_esi:            .space 8

.globl launch_code_end
launch_code_end:	


