#ident	"@(#)kern-i386at:psm/toolkits/at_toolkit/at_toolkit.c	1.1.2.1"
#ident	"$Header$"

/*
 * The AT toolkit supports:
 *
 *  	1) PSM assertion failure handling
 *  	2) PSM soft reboot
 *	3) PSM warm reset
 */

#include <svc/psm.h>
#include <psm/toolkits/psm_inline.h>
#include <psm/toolkits/at_toolkit/at_toolkit.h>

/*
 * void
 * psm_assfail(char*, char *, int)
 *      Called to procession ASSERT failures in PSM code.
 *	For now, just print the message & hang.
 *
 * Calling/Exit State:
 *      None.
 */

void
psm_assfail(unsigned char* a, char* f, int l)
{

	os_printf ("PSM ASSERTION failure: %s. file %s, line %d", a, f, l);
	asm (" int $3");
	asm (" hlt");
}


/*
 * void
 * psm_softreset(unsigned char*)
 *      Indicate that the subsequent reboot is a "soft" reboot.
 *
 * Calling/Exit State:
 *      None.
 */

void
psm_softreset(unsigned char* kvpage0)
{
        /* do soft reboot; only do memory check after power-on */

        *((unsigned long *)&kvpage0[0x467]) =
              ((unsigned long)AT_BIOS_SEG << 16) | (unsigned long)AT_BIOS_INIT;
        *((unsigned short *)&kvpage0[0x472]) = AT_RESET_FLAG;

        /* set shutdown flag to reset using int 19 */
        outb(AT_CMOS_ADDR, AT_SOFT_ADDR);
        outb(AT_CMOS_DATA, AT_SOFT_RESET);

}

/*
 * void
 * psm_warmreset()
 *      Indicate warm reset is in progress.
 *
 * Calling/Exit State:
 *      None.
 */

void
psm_warmreset()
{
        outb(AT_CMOS_ADDR, AT_SHUTDOWN_ADDR);
        outb(AT_CMOS_DATA, AT_WARM_RESET);
}

/*
 * void
 * psm_sysreset()
 *      Reset the system by sending a reset command to the keyboard.
 *
 * Calling/Exit State:
 *
 */

void
psm_sysreset()
{
        outb(AT_KB_CMD_ADDR, AT_KB_RESET);
}

