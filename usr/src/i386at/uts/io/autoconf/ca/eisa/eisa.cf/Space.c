#ident	"@(#)kern-i386at:io/autoconf/ca/eisa/eisa.cf/Space.c	1.3"
#ident	"$Header$"

#include <sys/param.h>
#include <sys/types.h>

/*
 * Bitmask of EISA slots to be left unparsed. 
 */
uint_t eisa_slot_mask = 0x01;

/*
 * Configuration Space access type -- real or protected mode.
 * According to the EISA spec., int 15 is a bimodal bios call, but
 * this is not supported on all EISA platforms. The <ca_eisa_realmode>
 * can be set to 0, if the system supports protected mode bios call.
 * The CA will then explicitly make the bios call to read NVRAM,
 * otherwise it will physmap the read NVRAM while the system was
 * in real-mode during early startup.
 */
int ca_eisa_realmode = 1;

/*
 * Bitmask of DMA channels to be left unparsed.
 */
uint_t eisa_dma_mask = 0x04;
