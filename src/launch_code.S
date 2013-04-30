	
.align 8
.globl launch_code_start
launch_code_start:
	movq %rax, %cr3
	jmp *%rcx
	hlt
.globl launch_code_end
launch_code_end:	


