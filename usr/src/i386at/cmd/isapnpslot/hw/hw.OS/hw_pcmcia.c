/*
 * File hw_pcmcia.c
 * Information handler for pcmcia
 *
 * @(#) hw_pcmcia.c 65.1 97/06/02 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>

#include "hw_pcmcia.h"
#include "hw_util.h"
#include "pcmcia_data.h"

const char	* const callme_pcmcia[] =
{
    "pcmcia",
    "pcmcia_bus",
    "pcmcia_buss",
    NULL
};

const char	short_help_pcmcia[] = "Info on PCMCIA bus devices";

static void	show_pcmcia_slot(FILE *out, u_char slot);
static u_char	pcmcia_read(u_char slot, u_char index);

/*
 * We assume that we have PCMCIA if we can get the revision from
 * the first slot.
 */

int
have_pcmcia(void)
{
    u_char	pcicRev = pcmcia_read(0, PCIC_REVISION) & 0x0f;

    return ((pcicRev == 0x00) || (pcicRev == 0x0f)) ? 0 : 1;
}

void
report_pcmcia(FILE *out)
{
    u_short	slot;

    report_when(out, "PCMCIA");

    if (!have_pcmcia())
    {
	fprintf(out, "    No PCMCIA Bus found\n");
	return;
    }

    load_pcmcia_data();

    for (slot = 0; slot < (u_short)(2 * PCIC_SLOTS); ++slot)
	show_pcmcia_slot(out, slot);
}

static void
show_pcmcia_slot(FILE *out, u_char slot)
{
    u_char	pcicRev;
    u_char	pcicStatus;

    pcicRev = pcmcia_read(slot, PCIC_REVISION);
    if (((pcicRev & 0x0f) == 0x00) || ((pcicRev & 0x0f) == 0x0f))
	return;		/* Not an active slot */

    fprintf(out, "    Slot: %u\n", (u_int)slot+1);
    fprintf(out, "\tPCMCIA Rev: %u\n", (u_int)pcicRev & 0x0f);

    pcicStatus = pcmcia_read(slot, PCIC_INTERFACE_STATUS);
    fprintf(out, "\tStatus:     0x%2.2x\n", (u_int)pcicStatus);
    fprintf(out, "\t    Card detect 1: %s\n", (pcicStatus & 0x08) ? Yes : No);
    fprintf(out, "\t    Card detect 2: %s\n", (pcicStatus & 0x04) ? Yes : No);

    /* ## more */
}

/*
 * Read Intel 82365SL PCIC slot space
 *
 * Slots are zero base numbered but we will be printing slots as
 * one based.
 */

static u_char
pcmcia_read(u_char slot, u_char index)
{
    u_long slotAddr = (slot < PCIC_SLOTS) ? PCIC_ADDR_1 : PCIC_ADDR_2;
    u_char slotBase = (slot % PCIC_SLOTS) * PCIC_ADDRS;

    io_outb(slotAddr, slotBase + index);
    return io_inb(slotAddr + 1);
}

