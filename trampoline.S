#include"inc/mmu.h"

# switch cpu from 16-bit real mode to 32-bit protected mode, jump into kernel entry.


.set PROT_MODE_CSEG, 0x8         # kernel code segment selector, 1st entry in GDT
.set PROT_MODE_DSEG, 0x10        # kernel data segment selector, 2nd entry in GDT
.set CR0_PE_ON,      0x1         # protected mode enable flag

.globl gemini_trampoline
gemini_trampoline:
    .code16                     # Assemble for 16-bit mode
    cli                         # Disable interrupts
    cld                         # String operations increment

    # Clear data segment registers (DS, ES, SS).
    xorw    %ax,%ax             # Segment number zero
    movw    %ax,%ds             # -> Data Segment
    movw    %ax,%es             # -> Extra Segment
    movw    %ax,%ss             # -> Stack Segment

    # Load bootstrap GDT that makes virtual addresses 
    # identical to their physical addresses, so that the 
    # effective memory map does not change during the switch.
    lgdt    gdtdesc

    # Eable protected mode
    movl    %cr0, %eax
    orl     $CR0_PE_ON, %eax
    movl    %eax, %cr0

    # Jump to next instruction, but in 32-bit code segment.
    # Switches processor into 32-bit mode.
    ljmp    $PROT_MODE_CSEG, $protcseg

    .code32                     # Assemble for 32-bit mode
protcseg:
    # Set up the protected-mode data segment registers
    movw    $PROT_MODE_DSEG, %ax    # Our data segment selector
    movw    %ax, %ds                # -> DS: Data Segment
    movw    %ax, %es                # -> ES: Extra Segment
    movw    %ax, %fs                # -> FS
    movw    %ax, %gs                # -> GS
    movw    %ax, %ss                # -> SS: Stack Segment

# Set up the stack pointer and call into C.
    movl    $(tmpstacktop), %esp
    call    entry

# If bootmain returns (it shouldn't), loop.
spin:
    jmp spin

.data
# Bootstrap GDT
# SEG(type,base,lim)					\
.p2align 2                                # force 4 byte alignment
gdt:
    SEG_NULL                              # null seg
    SEG(STA_X|STA_R, 0x0, 0xffffffff)     # code seg
    SEG(STA_W, 0x0, 0xffffffff)           # data seg

gdtdesc:
    .word   0x17                            # sizeof(gdt) - 1
    .long   gdt                             # address gdt

    .p2align    PGSHIFT
    .global     tmpstack
tmpstack:
    .space      (8*PGSIZE)
    .globl      tmpstacktop
tmpstacktop:
