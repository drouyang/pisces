.data
.align 8
.globl launch_code_start
launch_code_start:
    movq launch_code_target_addr(%rip), %rax
    movq launch_code_rsp(%rip), %rsp
    movl launch_code_esi(%rip), %esi
    jmp *%rax
    hlt


.org launch_code_start+40
.globl launch_code_rsp
launch_code_rsp:            .space 8
.globl launch_code_esi
launch_code_esi:            .space 8
.globl launch_code_target_addr
launch_code_target_addr:    .space 8
.globl launch_code_end
launch_code_end:
