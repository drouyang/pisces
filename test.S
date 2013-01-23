#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/pgtable_types.h>
#include <asm/page_types.h>
#include <asm/msr.h>
#include <asm/segment.h>
#include <asm/processor-flags.h>

    .text
    .code16

    .balign PAGE_SIZE

ENTRY(gemini_start)
    movw $1234, %ax
    movw $5678, %bx
    movw $3434, %cx
    hlt
do_loop:
    jmp do_loop
