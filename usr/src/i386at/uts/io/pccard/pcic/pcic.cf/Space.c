#ident	"@(#)kern-i386at:io/pccard/pcic/pcic.cf/Space.c	1.1"
#include	<sys/types.h>
#include	<sys/kmem.h>
#include	<sys/dma.h>
#include	<sys/cmn_err.h>
#include	<config.h>

struct pcic_cardmap {
	char *name;
	int type;
};

#define MODEMTYPE 2


/* This structure is a fallback for PC Card modems which do not 
 * have a FUNCID tuple.   If no CISTPL_FUNCID is found when parsing
 * the cards tuples, the modem enabler will look through this list
 * of cards.   Currently the only known card to have this problem
 * is the older MEGAHERTZ XJ1144 modem.   If your modem fails to
 * initialize, and there is a message in the osmlog indicating 
 * that the modem does not have a FUNCID tuple, create
 * en entry for your modem in the list below.  The "name" of the 
 * failing modem is included in the "FUNCID not found" error message.
 */
struct pcic_cardmap cardmap[] = {
        "MEGAHERTZ-XJ1144", MODEMTYPE
};
int pcic_cardmap_entries = sizeof(cardmap) / sizeof(struct pcic_cardmap);

int pcic_cfgentry_override = -1;

