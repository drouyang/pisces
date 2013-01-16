#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/pgtable_types.h>
#include <asm/page_types.h>
#include <asm/msr.h>
#include <asm/segment.h>
#include <asm/processor-flags.h>
#include "realmode.h"

    .text
    .code16

    .balign PAGE_SIZE
ENTRY(gemini_start)
    movl $1234, %eax
do_loop:
    jmp do_loop
