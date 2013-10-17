.data
.align 8
.globl launch_code_start
launch_code_start:
    movq launch_code_target_addr(%rip), %rax
    movl launch_code_esi(%rip), %esi
    jmp *%rax
    hlt


.org launch_code_start+48
.globl launch_code_esi
launch_code_esi:            .space 8
.globl launch_code_target_addr
launch_code_target_addr:    .space 8
.globl launch_code_end
launch_code_end:
