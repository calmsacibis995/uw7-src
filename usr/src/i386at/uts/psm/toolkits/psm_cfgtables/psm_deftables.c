#ident	"@(#)kern-i386at:psm/toolkits/psm_cfgtables/psm_deftables.c	1.1.2.2"
#ident	"$Header$"

/*
 * Default MP tables
 */

#include <svc/psm.h>
#include <psm/toolkits/psm_cfgtables/psm_cfgtables.h>


unsigned char mpc_default_1[] = {
	'P','C','M','P',			/* signature */
	0,0, 1, 0,		/* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,	/* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,	/* product id string */
	0,0,0,0,		/* oem table pointer */
	0,0, 2+1+1+16+2,0,		/* entry count */
	0x00,0x00,0xe0,0xfe,	/* address of local apic */
	0,0,0,0,		/* res. */

	CFG_ET_PROC,	/* processor */
	0,		/* apic id == 0 */
	1,		/* apic type - 82489DX */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	CFG_ET_PROC,	/* processor */
	1,		/* apic id == 1 */
	1,		/* apic type - 82489DX */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	CFG_ET_BUS,	/* bus */
	0,		/* bus id */
	'I','S','A',' ',
	' ',' ',

	CFG_ET_IOAPIC,	/* i/o apic */
	2,		/* apic id */
	1,		/* apic type - 82489DX (not used) */
	1,		/* enabled */
	0x00,0x00,0xc0,0xfe,	/* address of i/o apic */

	CFG_ET_I_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all i/o apics, line 0 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,1,2,1,		/* src(0,1) -> i/o apic with id=2, line 1 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,0,2,2,		/* src(0,2) -> i/o apic with id=2, line 2 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,3,2,3,		/* src(0,3) -> i/o apic with id=2, line 3 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,4,2,4,		/* src(0,4) -> i/o apic with id=2, line 4 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,5,2,5,		/* src(0,5) -> i/o apic with id=2, line 5 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,6,2,6,		/* src(0,6) -> i/o apic with id=2, line 6 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,7,2,7,		/* src(0,7) -> i/o apic with id=2, line 7 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,8,2,8,		/* src(0,8) -> i/o apic with id=2, line 8 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,9,2,9,		/* src(0,9) -> i/o apic with id=2, line 9 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,10,2,10,		/* src(0,10) -> i/o apic with id=2, line 10 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,11,2,11,		/* src(0,11) -> i/o apic with id=2, line 11 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,12,2,12,		/* src(0,12) -> i/o apic with id=2, line 12 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,13,2,13,		/* src(0,13) -> i/o apic with id=2, line 13 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,14,2,14,		/* src(0,14) -> i/o apic with id=2, line 14 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,15,2,15,		/* src(0,15) -> i/o apic with id=2, line 15 */

	CFG_ET_L_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all local apics, line 0 */
	CFG_ET_L_INTR,1,0,0,	/* NMI */
	0,0,0xff,1,		/* src(0,0) -> all local apics, line 1 */
};

unsigned char mpc_default_2[] = {
	'P','C','M','P',			/* signature */
	0,0, 1, 0,		/* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,	/* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,	/* product id string */
	0,0,0,0,		/* oem table pointer */
	0,0, 2+1+1+14+2,0,		/* entry count */
	0x00,0x00,0xe0,0xfe,	/* address of local apic */
	0,0,0,0,		/* res. */

	CFG_ET_PROC,	/* processor */
	0,		/* apic id == 0 */
	1,		/* apic type - 82489DX */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	CFG_ET_PROC,	/* processor */
	1,		/* apic id == 1 */
	1,		/* apic type - 82489DX */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	CFG_ET_BUS,	/* bus */
	0,		/* bus id */
	'E','I','S','A',
	' ',' ',

	CFG_ET_IOAPIC,	/* i/o apic */
	2,		/* apic id */
	1,		/* apic type - 82489DX (not used) */
	1,		/* enabled */
	0x00,0x00,0xc0,0xfe,	/* address of i/o apic */

	CFG_ET_I_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all i/o apics, line 0 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,1,2,1,		/* src(0,1) -> i/o apic with id=2, line 1 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,3,2,3,		/* src(0,3) -> i/o apic with id=2, line 3 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,4,2,4,		/* src(0,4) -> i/o apic with id=2, line 4 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,5,2,5,		/* src(0,5) -> i/o apic with id=2, line 5 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,6,2,6,		/* src(0,6) -> i/o apic with id=2, line 6 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,7,2,7,		/* src(0,7) -> i/o apic with id=2, line 7 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,8,2,8,		/* src(0,8) -> i/o apic with id=2, line 8 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,9,2,9,		/* src(0,9) -> i/o apic with id=2, line 9 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,10,2,10,		/* src(0,10) -> i/o apic with id=2, line 10 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,11,2,11,		/* src(0,11) -> i/o apic with id=2, line 11 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,12,2,12,		/* src(0,12) -> i/o apic with id=2, line 12 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,14,2,14,		/* src(0,14) -> i/o apic with id=2, line 14 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,15,2,15,		/* src(0,15) -> i/o apic with id=2, line 15 */

	CFG_ET_L_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all local apics, line 0 */
	CFG_ET_L_INTR,1,0,0,	/* NMI */
	0,0,0xff,1,		/* src(0,0) -> all local apics, line 1 */
};

unsigned char mpc_default_3[] = {
	'P','C','M','P',			/* signature */
	0,0, 1, 0,		/* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,	/* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,	/* product id string */
	0,0,0,0,		/* oem table pointer */
	0,0, 2+1+1+16+2,0,		/* entry count */
	0x00,0x00,0xe0,0xfe,	/* address of local apic */
	0,0,0,0,		/* res. */

	CFG_ET_PROC,	/* processor */
	0,		/* apic id == 0 */
	1,		/* apic type - 82489DX */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	CFG_ET_PROC,	/* processor */
	1,		/* apic id == 1 */
	1,		/* apic type - 82489DX */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	CFG_ET_BUS,	/* bus */
	0,		/* bus id */
	'E','I','S','A',
	' ',' ',

	CFG_ET_IOAPIC,	/* i/o apic */
	2,		/* apic id */
	1,		/* apic type - 82489DX (not used) */
	1,		/* enabled */
	0x00,0x00,0xc0,0xfe,	/* address of i/o apic */

	CFG_ET_I_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all i/o apics, line 0 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,1,2,1,		/* src(0,1) -> i/o apic with id=2, line 1 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,0,2,2,		/* src(0,2) -> i/o apic with id=2, line 2 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,3,2,3,		/* src(0,3) -> i/o apic with id=2, line 3 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,4,2,4,		/* src(0,4) -> i/o apic with id=2, line 4 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,5,2,5,		/* src(0,5) -> i/o apic with id=2, line 5 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,6,2,6,		/* src(0,6) -> i/o apic with id=2, line 6 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,7,2,7,		/* src(0,7) -> i/o apic with id=2, line 7 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,8,2,8,		/* src(0,8) -> i/o apic with id=2, line 8 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,9,2,9,		/* src(0,9) -> i/o apic with id=2, line 9 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,10,2,10,		/* src(0,10) -> i/o apic with id=2, line 10 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,11,2,11,		/* src(0,11) -> i/o apic with id=2, line 11 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,12,2,12,		/* src(0,12) -> i/o apic with id=2, line 12 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,13,2,13,		/* src(0,13) -> i/o apic with id=2, line 13 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,14,2,14,		/* src(0,14) -> i/o apic with id=2, line 14 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,15,2,15,		/* src(0,15) -> i/o apic with id=2, line 15 */

	CFG_ET_L_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all local apics, line 0 */
	CFG_ET_L_INTR,1,0,0,	/* NMI */
	0,0,0xff,1,		/* src(0,0) -> all local apics, line 1 */
};

unsigned char mpc_default_4[] = {
	'P','C','M','P',			/* signature */
	0,0, 1, 0,		/* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,	/* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,	/* product id string */
	0,0,0,0,		/* oem table pointer */
	0,0, 2+1+1+16+2,0,		/* entry count */
	0x00,0x00,0xe0,0xfe,	/* address of local apic */
	0,0,0,0,		/* res. */

	CFG_ET_PROC,	/* processor */
	0,		/* apic id == 0 */
	1,		/* apic type - 82489DX */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	CFG_ET_PROC,	/* processor */
	1,		/* apic id == 1 */
	1,		/* apic type - 82489DX */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	CFG_ET_BUS,	/* bus */
	0,		/* bus id */
	'M','C','A',' ',
	' ',' ',

	CFG_ET_IOAPIC,	/* i/o apic */
	2,		/* apic id */
	1,		/* apic type - 82489DX (not used) */
	1,		/* enabled */
	0x00,0x00,0xc0,0xfe,	/* address of i/o apic */

	CFG_ET_I_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all i/o apics, line 0 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,1,2,1,		/* src(0,1) -> i/o apic with id=2, line 1 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,0,2,2,		/* src(0,2) -> i/o apic with id=2, line 2 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,3,2,3,		/* src(0,3) -> i/o apic with id=2, line 3 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,4,2,4,		/* src(0,4) -> i/o apic with id=2, line 4 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,5,2,5,		/* src(0,5) -> i/o apic with id=2, line 5 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,6,2,6,		/* src(0,6) -> i/o apic with id=2, line 6 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,7,2,7,		/* src(0,7) -> i/o apic with id=2, line 7 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,8,2,8,		/* src(0,8) -> i/o apic with id=2, line 8 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,9,2,9,		/* src(0,9) -> i/o apic with id=2, line 9 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,10,2,10,		/* src(0,10) -> i/o apic with id=2, line 10 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,11,2,11,		/* src(0,11) -> i/o apic with id=2, line 11 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,12,2,12,		/* src(0,12) -> i/o apic with id=2, line 12 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,13,2,13,		/* src(0,13) -> i/o apic with id=2, line 13 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,14,2,14,		/* src(0,14) -> i/o apic with id=2, line 14 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,15,2,15,		/* src(0,15) -> i/o apic with id=2, line 15 */

	CFG_ET_L_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all local apics, line 0 */
	CFG_ET_L_INTR,1,0,0,	/* NMI */
	0,0,0xff,1,		/* src(0,0) -> all local apics, line 1 */
};

unsigned char mpc_default_5[] = {
	'P','C','M','P',			/* signature */
	0,0, 1, 0,		/* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,	/* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,	/* product id string */
	0,0,0,0,		/* oem table pointer */
	0,0, 2+2+1+16+2,0,		/* entry count */
	0x00,0x00,0xe0,0xfe,	/* address of local apic */
	0,0,0,0,		/* res. */

	CFG_ET_PROC,	/* processor */
	0,		/* apic id == 0 */
	0x10,		/* apic type - Integrated */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	CFG_ET_PROC,	/* processor */
	1,		/* apic id == 1 */
	0x10,		/* apic type - Integrated */
	1,		/* enable */
	0,0x20,0,0,	/* stepping, model, family, type=CM */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	CFG_ET_BUS,	/* bus */
	0,		/* bus id */
	'I','S','A',' ',
	' ',' ',

	CFG_ET_BUS,	/* bus */
	1,		/* bus id */
	'P','C','I',' ',
	' ',' ',

	CFG_ET_IOAPIC,	/* i/o apic */
	2,		/* apic id */
	0x10,		/* apic type - Integrated (not used) */
	1,		/* enabled */
	0x00,0x00,0xc0,0xfe,	/* address of i/o apic */

	CFG_ET_I_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all i/o apics, line 0 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,1,2,1,		/* src(0,1) -> i/o apic with id=2, line 1 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,0,2,2,		/* src(0,2) -> i/o apic with id=2, line 2 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,3,2,3,		/* src(0,3) -> i/o apic with id=2, line 3 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,4,2,4,		/* src(0,4) -> i/o apic with id=2, line 4 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,5,2,5,		/* src(0,5) -> i/o apic with id=2, line 5 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,6,2,6,		/* src(0,6) -> i/o apic with id=2, line 6 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,7,2,7,		/* src(0,7) -> i/o apic with id=2, line 7 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,8,2,8,		/* src(0,8) -> i/o apic with id=2, line 8 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,9,2,9,		/* src(0,9) -> i/o apic with id=2, line 9 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,10,2,10,		/* src(0,10) -> i/o apic with id=2, line 10 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,11,2,11,		/* src(0,11) -> i/o apic with id=2, line 11 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,12,2,12,		/* src(0,12) -> i/o apic with id=2, line 12 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,13,2,13,		/* src(0,13) -> i/o apic with id=2, line 13 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,14,2,14,		/* src(0,14) -> i/o apic with id=2, line 14 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,15,2,15,		/* src(0,15) -> i/o apic with id=2, line 15 */

	CFG_ET_L_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all local apics, line 0 */
	CFG_ET_L_INTR,1,0,0,	/* NMI */
	0,0,0xff,1,		/* src(0,0) -> all local apics, line 1 */
};

unsigned char mpc_default_6[] = {
	'P','C','M','P',			/* signature */
	0,0, 1, 0,		/* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,	/* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,	/* product id string */
	0,0,0,0,		/* oem table pointer */
	0,0, 2+2+1+16+2,0,		/* entry count */
	0x00,0x00,0xe0,0xfe,	/* address of local apic */
	0,0,0,0,		/* res. */

	CFG_ET_PROC,	/* processor */
	0,		/* apic id == 0 */
	0x10,		/* apic type - Integrated */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	CFG_ET_PROC,	/* processor */
	1,		/* apic id == 1 */
	0x10,		/* apic type - Integrated */
	1,		/* enable */
	0,0x20,0,0,	/* stepping, model, family, type=CM */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	CFG_ET_BUS,	/* bus */
	0,		/* bus id */
	'E','I','S','A',
	' ',' ',

	CFG_ET_BUS,	/* bus */
	1,		/* bus id */
	'P','C','I',' ',
	' ',' ',

	CFG_ET_IOAPIC,	/* i/o apic */
	2,		/* apic id */
	0x10,		/* apic type - Integrated (not used) */
	1,		/* enabled */
	0x00,0x00,0xc0,0xfe,	/* address of i/o apic */

	CFG_ET_I_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all i/o apics, line 0 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,1,2,1,		/* src(0,1) -> i/o apic with id=2, line 1 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,0,2,2,		/* src(0,2) -> i/o apic with id=2, line 2 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,3,2,3,		/* src(0,3) -> i/o apic with id=2, line 3 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,4,2,4,		/* src(0,4) -> i/o apic with id=2, line 4 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,5,2,5,		/* src(0,5) -> i/o apic with id=2, line 5 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,6,2,6,		/* src(0,6) -> i/o apic with id=2, line 6 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,7,2,7,		/* src(0,7) -> i/o apic with id=2, line 7 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,8,2,8,		/* src(0,8) -> i/o apic with id=2, line 8 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,9,2,9,		/* src(0,9) -> i/o apic with id=2, line 9 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,10,2,10,		/* src(0,10) -> i/o apic with id=2, line 10 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,11,2,11,		/* src(0,11) -> i/o apic with id=2, line 11 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,12,2,12,		/* src(0,12) -> i/o apic with id=2, line 12 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,13,2,13,		/* src(0,13) -> i/o apic with id=2, line 13 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,14,2,14,		/* src(0,14) -> i/o apic with id=2, line 14 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,15,2,15,		/* src(0,15) -> i/o apic with id=2, line 15 */

	CFG_ET_L_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all local apics, line 0 */
	CFG_ET_L_INTR,1,0,0,	/* NMI */
	0,0,0xff,1,		/* src(0,0) -> all local apics, line 1 */
};

unsigned char mpc_default_7[] = {
	'P','C','M','P',			/* signature */
	0,0, 1, 0,		/* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,	/* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,	/* product id string */
	0,0,0,0,		/* oem table pointer */
	0,0, 2+2+1+15+2,0,		/* entry count */
	0x00,0x00,0xe0,0xfe,	/* address of local apic */
	0,0,0,0,		/* res. */

	CFG_ET_PROC,	/* processor */
	0,		/* apic id == 0 */
	0x10,		/* apic type - Integrated */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	CFG_ET_PROC,	/* processor */
	1,		/* apic id == 1 */
	0x10,		/* apic type - Integrated */
	1,		/* enable */
	0,0x20,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	CFG_ET_BUS,	/* bus */
	0,		/* bus id */
	'M','C','A',' ',
	' ',' ',

	CFG_ET_BUS,	/* bus */
	1,		/* bus id */
	'P','C','I',' ',
	' ',' ',

	CFG_ET_IOAPIC,	/* i/o apic */
	2,		/* apic id */
	0x10,		/* apic type - Integrated (not used) */
	1,		/* enabled */
	0x00,0x00,0xc0,0xfe,	/* address of i/o apic */

	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,1,2,1,		/* src(0,1) -> i/o apic with id=2, line 1 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,0,2,2,		/* src(0,2) -> i/o apic with id=2, line 2 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,3,2,3,		/* src(0,3) -> i/o apic with id=2, line 3 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,4,2,4,		/* src(0,4) -> i/o apic with id=2, line 4 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,5,2,5,		/* src(0,5) -> i/o apic with id=2, line 5 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,6,2,6,		/* src(0,6) -> i/o apic with id=2, line 6 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,7,2,7,		/* src(0,7) -> i/o apic with id=2, line 7 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,8,2,8,		/* src(0,8) -> i/o apic with id=2, line 8 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,9,2,9,		/* src(0,9) -> i/o apic with id=2, line 9 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,10,2,10,		/* src(0,10) -> i/o apic with id=2, line 10 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,11,2,11,		/* src(0,11) -> i/o apic with id=2, line 11 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,12,2,12,		/* src(0,12) -> i/o apic with id=2, line 12 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,13,2,13,		/* src(0,13) -> i/o apic with id=2, line 13 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,14,2,14,		/* src(0,14) -> i/o apic with id=2, line 14 */
	CFG_ET_I_INTR,0,0,0,	/* INTR */
	0,15,2,15,		/* src(0,15) -> i/o apic with id=2, line 15 */

	CFG_ET_L_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all local apics, line 0 */
	CFG_ET_L_INTR,1,0,0,	/* NMI */
	0,0,0xff,1,		/* src(0,0) -> all local apics, line 1 */
};

/*	NO_IO_APIC_OPTION	*/
unsigned char mpc_default_8[] = {
	'P','C','M','P',			/* signature */
	0,0, 1, 0,		/* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,	/* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,	/* product id string */
	0,0,0,0,		/* oem table pointer */
	0,0, 2+1+0+0+2,0,		/* entry count */
	0x00,0x00,0xe0,0xfe,	/* address of local apic */
	0,0,0,0,		/* res. */

	CFG_ET_PROC,	/* processor */
	0,		/* apic id == 0 */
	0x10,		/* apic type - Integrated */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	CFG_ET_PROC,	/* processor */
	1,		/* apic id == 1 */
	0x10,		/* apic type - Integrated */
	1,		/* enable */
	0,0x20,0,0,	/* stepping, model, family, type=CM */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	CFG_ET_BUS,	/* bus */
	0,		/* bus id */
	'I','S','A',' ',
	' ',' ',

	CFG_ET_L_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all local apics, line 0 */
	CFG_ET_L_INTR,1,0,0,	/* NMI */
	0,0,0xff,1,		/* src(0,0) -> all local apics, line 1 */
};
/*	NO_IO_APIC_OPTION */


struct mpconfig * mpc_defaults[] = {
	0,
	(struct mpconfig *)mpc_default_1,
	(struct mpconfig *)mpc_default_2,
	(struct mpconfig *)mpc_default_3,
	(struct mpconfig *)mpc_default_4,
	(struct mpconfig *)mpc_default_5,
	(struct mpconfig *)mpc_default_6,
	(struct mpconfig *)mpc_default_7,
	(struct mpconfig *)mpc_default_8,
	0
};

int mp_entry_sizes[] = {
        sizeof(struct mpe_proc),
        sizeof(struct mpe_bus),
        sizeof(struct mpe_ioapic),
        sizeof(struct mpe_intr),
        sizeof(struct mpe_intr),
};

