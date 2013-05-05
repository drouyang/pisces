	
.align 8
.globl launch_code_start
launch_code_start:
    movq %rax, %cr3
    jmp *%rcx
    hlt

.balign 8
.globl launch_code_header_asm
launch_code_header_asm:
    kernel_addr:            .space 8
    real_mode_data_addr:    .space 8


.globl launch_code_end
launch_code_end:	


