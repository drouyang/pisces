#include <linux/init.h>
#include <linux/smp.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/percpu.h>
#include <linux/bootmem.h>
#include <linux/err.h>
#include <linux/nmi.h>
#include <linux/tboot.h>
#include <linux/stackprotector.h>
#include <linux/gfp.h>
#include <linux/cpuidle.h>

#include <asm/acpi.h>
#include <asm/desc.h>
#include <asm/nmi.h>
#include <asm/irq.h>
#include <asm/idle.h>
#include <asm/realmode.h>
#include <asm/cpu.h>
#include <asm/numa.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>
#include <asm/mtrr.h>
#include <asm/mwait.h>
#include <asm/apic.h>
#include <asm/io_apic.h>
#include <asm/i387.h>
#include <asm/fpu-internal.h>
#include <asm/setup.h>
#include <asm/uv/uv.h>
#include <linux/mc146818rtc.h>

#include <asm/smpboot_hooks.h>
#include <asm/i8259.h>

#include <asm/realmode.h>

#include "linux_syms.h"

int lapic_get_maxlvt(void)
{
    unsigned int v;

    v = apic_read(APIC_LVR);

    return APIC_INTEGRATED(GET_APIC_VERSION(v)) ? GET_APIC_MAXLVT(v) : 2;
}
int wakeup_secondary_cpu_via_init(int phys_apicid, unsigned long start_eip)
{
        unsigned long send_status, accept_status = 0;
        int maxlvt, num_starts, j;

        maxlvt = lapic_get_maxlvt();

        /*
         * Be paranoid about clearing APIC errors.
         */
        if (APIC_INTEGRATED(apic_version[phys_apicid])) {
                if (maxlvt > 3)         /* Due to the Pentium erratum 3AP.  */
                        apic_write(APIC_ESR, 0);
                apic_read(APIC_ESR);
        }

        pr_debug("Asserting INIT\n");

        /*
         * Turn INIT on target chip
         */
        /*
         * Send IPI
         */
        apic_icr_write(APIC_INT_LEVELTRIG | APIC_INT_ASSERT | APIC_DM_INIT,
                       phys_apicid);

        pr_debug("Waiting for send to finish...\n");
        send_status = safe_apic_wait_icr_idle();

        mdelay(10);

        pr_debug("Deasserting INIT\n");

        /* Target chip */
        /* Send IPI */
        apic_icr_write(APIC_INT_LEVELTRIG | APIC_DM_INIT, phys_apicid);

        pr_debug("Waiting for send to finish...\n");
        send_status = safe_apic_wait_icr_idle();

        mb();
        //atomic_set(&init_deasserted, 1);

        /*
         * Should we send STARTUP IPIs ?
         *
         * Determine this based on the APIC version.
         * if we don't have an integrated APIC, don't send the STARTUP IPIs.
         */
        if (APIC_INTEGRATED(apic_version[phys_apicid]))
                num_starts = 2;
        else
                num_starts = 0;

        /*
         * Paravirt / VMI wants a startup IPI hook here to set up the
         * target processor state.
         */
        startup_ipi_hook(phys_apicid, (unsigned long) linux_start_secondary_addr,
                         stack_start);

        /*
         * Run STARTUP IPI loop.
         */
        pr_debug("#startup loops: %d\n", num_starts);

        for (j = 1; j <= num_starts; j++) {
                pr_debug("Sending STARTUP #%d\n", j);
                if (maxlvt > 3)         /* Due to the Pentium erratum 3AP.  */
                        apic_write(APIC_ESR, 0);
                apic_read(APIC_ESR);
                pr_debug("After apic_write\n");

                /*
                 * STARTUP IPI
                 */

                /* Target chip */
                /* Boot on the stack */
                /* Kick the second */
                apic_icr_write(APIC_DM_STARTUP | (start_eip >> 12),
                               phys_apicid);

                /*
                 * Give the other CPU some time to accept the IPI.
                 */
                udelay(300);

                pr_debug("Startup point 1\n");

                pr_debug("Waiting for send to finish...\n");
                send_status = safe_apic_wait_icr_idle();

                /*
                 * Give the other CPU some time to accept the IPI.
                 */
                udelay(200);
                if (maxlvt > 3)         /* Due to the Pentium erratum 3AP.  */
                       apic_write(APIC_ESR, 0);
                accept_status = (apic_read(APIC_ESR) & 0xEF);
                if (send_status || accept_status)
                        break;
        }
        pr_debug("After Startup\n");

        if (send_status)
                pr_err("APIC never delivered???\n");
        if (accept_status)
                pr_err("APIC delivery error (%lx)\n", accept_status);

        return (send_status | accept_status);
}

