#include <linux/linkage.h>

#include <asm/cache.h>
#include <asm/errno.h>
#include <asm/dwarf2.h>
#include <asm/calling.h>
#include <asm/asm-offsets.h>
#include <asm/msr.h>
#include <asm/unistd.h>
#include <asm/thread_info.h>
#include <asm/hw_irq.h>
#include <asm/page_types.h>
#include <asm/irqflags.h>
#include <asm/paravirt.h>
#include <asm/percpu.h>
#include <asm/asm.h>
#include <asm/context_tracking.h>
#include <asm/smap.h>
#include <asm/pgtable_types.h>
#include <linux/err.h>



/* BJK: This is borrowed directly from <kitten>/arch/x86/kernel/entry.S */
ENTRY(asm_interrupt)

    cld				/* Clear direction flag              */

    /*
     * Save registers to kernel-stack
     */
    subq $14*8,           %rsp	/* Make room on the stack            */
    movq %rsi,       13*8(%rsp)
    movq 14*8(%rsp),      %rsi	/* ARG1: the interrupt vector number */
    movq %rdi,       14*8(%rsp)
    movq %rdx,       12*8(%rsp)
    movq %rcx,       11*8(%rsp)
    movq %rax,       10*8(%rsp)
    movq %r8,        9*8(%rsp)
    movq %r9,        8*8(%rsp)
    movq %r10,       7*8(%rsp) 
    movq %r11,       6*8(%rsp) 
    movq %rbx,       5*8(%rsp)
    movq %rbp,       4*8(%rsp)
    movq %r12,       3*8(%rsp)
    movq %r13,       2*8(%rsp)
    movq %r14,       1*8(%rsp)
    movq %r15,          (%rsp)

    testl $3, CS(%rsp)
    je 1f
    swapgs

1:

    movq %rsp, %rdi             /* ARG0: pointer to 'struct pt_regs' */
    call do_generic_ipi_handler	/* Call common C handler             */

    testl $3, CS(%rsp)
    je 2f
    swapgs

2:
    /*
     * Restore registers and return to interrupted program
     */
    movq     (%rsp), %r15
    movq  1*8(%rsp), %r14
    movq  2*8(%rsp), %r13
    movq  3*8(%rsp), %r12
    movq  4*8(%rsp), %rbp
    movq  5*8(%rsp), %rbx
    movq  6*8(%rsp), %r11
    movq  7*8(%rsp), %r10
    movq  8*8(%rsp), %r9
    movq  9*8(%rsp), %r8
    movq 10*8(%rsp), %rax
    movq 11*8(%rsp), %rcx
    movq 12*8(%rsp), %rdx
    movq 13*8(%rsp), %rsi
    movq 14*8(%rsp), %rdi

    /* Add 16*8 to pop the error code and vector number pushed in the idt
       function stubs below
    */
    addq $16*8,      %rsp

    iretq
END(asm_interrupt)


.align 16
ENTRY(generic_ipi_table)
    vector=0
    .rept NR_VECTORS
	.if vector<=7||vector==9||vector==15||vector==16||vector>=18
	    pushq $0      /* push dummy error_code */
	.endif
	pushq $vector     /* push vector # into RDI slot */
	jmp asm_interrupt /* call common handler */

	/* Move onto next entry in table*/
	.align 16
	vector=vector+1
    .endr
END(generic_ipi_table)
