#ident	"@(#)kern-i386at:io/autoconf/ca/eisa/eisanvm.c	1.12"
#ident	"$Header$"

/*
 * EISA non-volatile memory access functions. These functions can be 
 * called anytime during run-time on systems where it is permissible
 * to make protected-mode BIOS calls. 
 */

#include <proc/regset.h>
#include <svc/eisa.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>

#include <io/autoconf/ca/ca.h>
#include <io/autoconf/ca/eisa/nvm.h>


#if defined(DEBUG) || defined(DEBUG_TOOLS)
STATIC int eisa_debug = 0;
#define DEBUG1(a)	if (eisa_debug == 1) printf a
#define DEBUG2(a)	if (eisa_debug == 2) printf a
#else
#define DEBUG1(a)
#define DEBUG2(a)
#endif /* DEBUG || DEBUG_TOOLS */

extern int _eisa_rom_call(struct regs *, caddr_t);
extern int ca_eisa_access_type(void);

int	eisa_init_bios_entry(void);

caddr_t eisa_bios_vaddr;		/* virtual address of bios call */


/*
 * int
 * eisa_init_bios_entry(void)
 *
 * Calling/Exit State:
 *	Returns ENOMEM if not enough virtual space, otherwise return 0.
 */
int
eisa_init_bios_entry(void)
{
	eisa_bios_vaddr = physmap(0xf0000, 0xffff, KM_NOSLEEP);
	if (eisa_bios_vaddr == NULL) {
		/*
		 *+ Not enough virtual space. Check memory configured
		 *+ in your system.
		 */
		cmn_err(CE_WARN, 
			"!eisa_init_bios_entry: Not enough virtual space");
		return (ENOMEM);
	} else {
		eisa_bios_vaddr += 0xf859;
	}

	return (0);
}


/*
 * int
 * eisa_boardid(int slot, char *buffer)
 *	Read eisa board id. information from cmos memory.
 *
 * Calling/Exit State:
 *	<slot> is the eisa socket no. in which the board is plugged.
 *	<buffer> contains the EISA board identification string. 
 */
int
eisa_boardid(int slot, char *buffer)
{
	regs reg;
	int status;

	if (ca_eisa_access_type()) {
		buffer[0] = inb(EISA_ID0(slot));
		buffer[1] = inb(EISA_ID1(slot));
		buffer[2] = inb(EISA_ID2(slot));
		buffer[3] = inb(EISA_ID3(slot));
		return (0);
	} else {
		bzero(&reg, sizeof(regs));
		reg.eax.word.ax = EISA_READ_SLOT;
		reg.ecx.byte.cl = slot;

		status = eisa_rom_call(&reg);

		*((short *)buffer)++ = reg.edi.word.di;
		*((short *)buffer)++ = reg.esi.word.si;
		return (status);
	}
}


/*
 * int
 * eisa_clear_nvm(uchar_t majrevno, uchar_t minrevno)
 *	Clear eisa board information from cmos memory.
 *
 * Calling/Exit State:
 *	<majrevno> is the configuration utility major revision level.
 *	<minrevno> is the configuration utility minor revision level.
 *
 * Remarks:
 *	This BIOS routine call clears all EISA nonvolatile memory
 *	locations.
 */
int
eisa_clear_nvm(uchar_t majrevno, uchar_t minrevno)
{
	regs reg;
	int status;

	bzero(&reg, sizeof(regs));
	reg.eax.word.ax = EISA_CLEAR_NVM;
	reg.ecx.byte.cl = minrevno;
	reg.ecx.byte.ch = majrevno;

	status = eisa_rom_call(&reg);

	return(status);
}


/*
 * int
 * eisa_write_config(char *buffer, size_t count)
 *	Write slot data from eisa cmos memory.
 *
 * Calling/Exit Status:
 *	<slot> is the eisa socket no. in which the board is plugged.
 */
int
eisa_write_config(char *buffer, size_t count)
{
	regs reg;
	int status;

	bzero(&reg, sizeof(regs));
	reg.eax.word.ax = EISA_WRITE_SLOT;
	reg.ecx.word.cx = count;
	reg.esi.esi = (unsigned int)buffer;

	status = eisa_rom_call(&reg);

	return(status);
}


/*
 * int
 * eisa_read_slot(int slot, char *buffer)
 *	Read slot data from eisa cmos memory.
 *
 * Calling/Exit Status:
 *	<slot> is the eisa socket no. in which the board is plugged.
 *	<buffer> is the slot data area.
 */
int
eisa_read_slot(int slot, char *buffer)
{
	regs reg;
	int status;

	bzero(&reg, sizeof(regs));
	reg.eax.word.ax = EISA_READ_SLOT;
	reg.ecx.byte.cl = slot;

	status = eisa_rom_call(&reg);

	/* Arranges data to match "NVM_SLOTINFO" structure. See "nvm.h". */

	*((short *)buffer)++ = reg.edi.word.di;
	*((short *)buffer)++ = reg.esi.word.si;
	*((short *)buffer)++ = reg.ebx.word.bx;
	*buffer++ = reg.edx.byte.dh;
	*buffer++ = reg.edx.byte.dl;
	*((short *)buffer)++ = reg.ecx.word.cx;
	*buffer++ = reg.eax.byte.al;
	*buffer++ = reg.eax.byte.ah;
	return(status);
}


/*
 * int
 * eisa_read_func(int slot, int func, char *buffer)
 *	Read function data from eisa cmos memory.
 *
 * Calling/Exit State:
 *	<slot> is the eisa socket no. in which the board is plugged.
 *	<func> is the function block number to read.
 *	<buffer> is the function data area.
 */
int
eisa_read_func(int slot, int func, char *buffer)
{
	regs reg;
	int status;

	bzero(&reg, sizeof(regs));
	reg.eax.word.ax = EISA_READ_FUNC;
	reg.ecx.byte.cl = slot;
	reg.ecx.byte.ch = func;
	reg.esi.esi = (unsigned int)buffer;

	status = eisa_rom_call(&reg);

	/* Data is arranged to match "NVM_FUNCINFO" structure. See "nvm.h". */

	return(status);
}


/*
 * int
 * eisa_parse_func(void *data, void *str, uint_t stype)
 *
 * Calling/Exit State:
 *	<data> is in the eisa_nvm_funcinfo_t format.
 *	<str> is the character string to be searched.
 *	<stype> is the type of data to be parsed.
 *
 * Remarks:
 *	Searches for the type/sub-type string specified.
 */
int
eisa_parse_func(void *data, void *val, uint_t stype)
{
	uchar_t	*type;
	eisa_nvm_funcinfo_t	*fip;
	char *str = (char *)val;


	fip = (eisa_nvm_funcinfo_t *)data; 
	type = fip->type;

	if ((stype & EISA_TYPE) || (stype == 0)) {

		while (*type && *type != ';' && type < fip->type + NVM_MAX_TYPE) {

			/* "type" points to a TYPE string. */

			if (strncmp((char *)type, str, strlen(str)) == 0) { 
				/* Found the type string */
				return (CA_SUCCESS);
			}

			/*
			 * Looks for the beginning of the next string.
			 * Note that ',' is a delimiter for TYPE string 
			 * fragments.
			 */
			while (*type && *type != ';' && type < fip->type + NVM_MAX_TYPE)
				if (*type++ == ',') 
					break;     
		} /* end while */

	} else {
		/*
		 * Skip over the TYPE string.
		 */
		while (*type && *type != ';' && type < fip->type + NVM_MAX_TYPE)
			type++;
	}

	 if ((stype & EISA_SUBTYPE) || (stype == 0)) {

		/*
		 * "sub-type" strings follow the ";" character.
		 * Note that the ';' is a delimiter for the SUBTYPE
		 * string.
		 */
		if (*type)
			type++;

		while (*type && type < fip->type + NVM_MAX_TYPE) {

			/* "type" points to a SUB-TYPE string */

			if (strncmp((char *)type, str, strlen(str)) == 0) { 
				/* Found the sub-type string */
				return (CA_SUCCESS);
			}

			/*
			 * Looks for the beginning of the next string.
			 * Note that ',' is a delimiter for SUBTYPE string 
			 * fragments.
			 *
			 * Note: The SUBTYPE can have multiple, semicolon
			 * delimited ASCII strings to the initial SUBTYPE.
			 */
			while (*type && type < fip->type + NVM_MAX_TYPE)
				if (*type++ == ',' || *type++ == ';') 
					break;     
		} /* end while */
	} 

	return (CA_FAILURE);
}


/*
 * void
 * eisa_print_errmsg(int status)
 *
 * Calling/Exit Status:
 *	<status> is error number indicated by the BIOS call.
 */
void
eisa_print_errmsg(int status)
{
	char *errmsg[] = {
		"NVM_SUCCESSFUL (0x00): No errors", 
		"NVM_INVALID_SLOT (0x80): Invalid slot number",
		"NVM_INVALID_FUNCTION (0x81): Invalid function number",
		"NVM_INVALID_CMOS (0x82): Nonvolatile memory corrupt",
		"NVM_EMPTY_SLOT (0x83): Slot is empty",
		"NVM_WRITE_ERROR (0x84): Failure to write to CMOS",
		"NVM_MEMORY_FULL (0x85): CMOS memory is full",
		"NVM_NOT_SUPPORTED (0x86): EISA CMOS not supported",
		"NVM_INVALID_SETUP (0x87): Invalid Setup information",
		"NVM_INVALID_VERSION (0x88): BIOS cannot support this version"
	};
	char *msg;

	if (status < 0)
		return;

	ASSERT(status >= NVM_INVALID_SLOT && status <= NVM_INVALID_VERSION);

	msg = errmsg[status & 0x0F];

	cmn_err(CE_NOTE, msg);
}


/*
 * void
 * eisa_print_slot(NVM_SLOTINFO *sp)
 *
 * Calling/Exit Status:
 *	<sp> is a pointer to the nvm_slotinfo_t data type.
 */
void
eisa_print_slot(NVM_SLOTINFO *sp)
{
#if defined(DEBUG) || defined(DEBUG_TOOLS)
	
	if (eisa_debug == 0)
		return;

	cmn_err(CE_CONT, "\n nvm_slotinfo struct: size=0x%x(%d)\n",
		sizeof(NVM_SLOTINFO), sizeof(NVM_SLOTINFO));
	cmn_err(CE_CONT, "\tboardid=%s (%s), \trevision=0x%x\n",
		sp->boardid, eisa_uncompress((char *)sp->boardid), sp->revision);
	cmn_err(CE_CONT, "\tfunctions=0x%x,\tfib=0x%x\n",
		sp->functions, sp->fib);
	cmn_err(CE_CONT, "\tchecksum=0x%x, \tdupid=0x%x\n",
		sp->checksum, sp->dupid);

#endif /* DEBUG || DEBUG_TOOLS */
}


/*
 * void
 * eisa_print_func(NVM_FUNCINFO *fp)
 *
 * Calling/Exit Status:
 *	<fp> is a pointer to the nvm_funcinfo_t data type.
 */
void
eisa_print_func(NVM_FUNCINFO *fp)
{
#if defined(DEBUG) || defined(DEBUG_TOOLS)

	if (eisa_debug == 0)
		return;

	cmn_err(CE_CONT, "\n nvm_funcinfo struct: size=0x%x(%d)\n",
		sizeof(NVM_FUNCINFO), sizeof(NVM_FUNCINFO));
	cmn_err(CE_CONT, "\tboardid=%s (%s), \tdupid=0x%x\n",
		fp->boardid, eisa_uncompress((char *)fp->boardid), fp->dupid);
	cmn_err(CE_CONT, "\tovl_minor=0x%1x,\tovl_minor=0x%1x\n",
		fp->ovl_minor, fp->ovl_major);
	cmn_err(CE_CONT, "\tfib=0x%x, \ttype=%s\n",
		fp->fib, fp->type);

	if (fp->fib.irq) {
		cmn_err(CE_CONT, "\tirq.line=0x%x, \tirq.trigger=%x\n",
			fp->u.r.irq[0].line, fp->u.r.irq[0].trigger);
		cmn_err(CE_CONT, "\tirq.share=0x%x, \tirq.more=%x\n",
			fp->u.r.irq[0].share, fp->u.r.irq[0].more);
	}

#endif /* DEBUG || DEBUG_TOOLS */
}


/*
 * char
 * itoh(int n)
 *      Convert an integer to an hexadecimal.
 *
 * Calling/Exit State:
 *      None.
 */
char
itoh(int n)
{
        return "0123456789ABCDEF"[n];
}


/*
 * char *
 * eisa_uncompress(char *name)
 *      Converts from 4 byte compressedID format
 *      to 7 byte ASCII format.
 *
 * Calling/Exit State:
 *      None.
 */
char *
eisa_uncompress(char *name)
{
        static char cbuf[8];

        cbuf[0] = 'A' - 1 + ((name[0] >> 2) & 0x001f);
        cbuf[1] = 'A' - 1 + (((name[0] << 3) & 0x18) | ((name[1] >> 5) & 0x07));
        cbuf[2] = 'A' - 1 + (name[1] & 0x001f);
        cbuf[3] = itoh((name[2] & 0xf0) >> 4);
        cbuf[4] = itoh(name[2] & 0x0f);
        cbuf[5] = itoh((name[3] & 0xf0) >> 4);
        cbuf[6] = itoh(name[3] & 0x0f);
        cbuf[7] = '\0';

        return cbuf;
}


/*
 * int
 * eisa_read_nvm(int slot, uchar_t *data, int *errorp)
 *	Fill the data buffer from eisa cmos memory.
 *
 * Calling/Exit State:
 *	<slot> is the EISA socket number of the device.
 *	<data> is a pointer to a buffer allocated by the caller.
 *	<errorp> is a return pointer to the slot status.
 *
 * Remarks:
 *	The "eisa_read_nvm" is a general-purpose function for extracting 
 *	configuration data from EISA non-volatile memory using protected
 *	mode BIOS calls.
 *
 *	The EISA NVRAM data is stored in the following format:
 *	
 *	---------------------------------
 *	|	Slot Information	|
 *	---------------------------------
 *	|	1st Function Info.	|
 *	---------------------------------
 *	|	2nd Function Info.	|
 *	---------------------------------
 *	|				|
 *	---------------------------------
 *	|	nth Function Info.	|
 *	---------------------------------
 */
int
eisa_read_nvm(int slot, uchar_t *data, int *errorp)
{
	unsigned char *dstart = data;	/* data start */
	short slimit;			/* slot limit */
	short function;			/* function number */
	short flimit;			/* function limit */
	int nfuncs = 0;			/* number of functions */
	int status;			/* status of a bios call */
	NVM_SLOTINFO slotinfo;		/* slot information */
	NVM_FUNCINFO funcinfo;		/* function information */


	ASSERT(slot < EISA_MAX_SLOTS);

	if ((status = eisa_read_slot(slot, (char *)&slotinfo)) == 0) {

		/* Handles request for a specific function. */

		bcopy(&slotinfo, data, sizeof(NVM_SLOTINFO));
		data += sizeof(NVM_SLOTINFO);

		flimit = (short)slotinfo.functions;

		DEBUG2(("eisa_read_nvm: Calling eisa_read_func:\n "
			"flimit(addr=0x%x)=%d, function(addr=0x%x)=%d\n",
			&flimit, flimit, &function, function));

		for (function = 0; function < flimit; function++) {

			if ((status = eisa_read_func(slot, (int)function, 
					(char *)&funcinfo)) == 0) {

				bcopy(&funcinfo, data, sizeof(NVM_FUNCINFO));
				data += sizeof(NVM_FUNCINFO);
				nfuncs++;

			} else {
				eisa_print_errmsg(status);
				*errorp = status;
			}
		}

		DEBUG2(("eisa_read_nam: Returning...\n nfuncs=%d "
			"flimit=%d, function=%d, slotinfo.functions=%d\n",
			nfuncs, flimit, function, slotinfo.functions));

		ASSERT(nfuncs == slotinfo.functions);
		ASSERT(function == flimit);

	} else {
		eisa_print_errmsg(status);
		*errorp = status;
	}

	return (nfuncs);
}


/*
 * int
 * eisa_parse_devconfig(void *data, void *str, uint_t dtype, uint_t stype)
 *	Parse for <str> in <data> block based on the qualifiers specified
 *	in <dtype> and <stype>.
 *
 * Calling/Exit State:
 *	<data> should be in the format saved by eisa_read_nvm.
 *	<str> is the character string to be searched.
 *	<dtype> is the type of data passed in the argument.
 *	<stype> is the data search type.
 *
 *	See comments above for the format of <data>.
 */
int
eisa_parse_devconfig(void *data, void *str, uint_t dtype, uint_t stype)
{
	int     f;        		      /* no. of functions */
	eisa_nvm_funcinfo_t    *funcp;
	eisa_nvm_slotinfo_t    *slotp;

	switch (dtype) {
	case EISA_SLOT_DATA:
		slotp = (eisa_nvm_slotinfo_t *)data;
		funcp = (eisa_nvm_funcinfo_t *)((char *)data + EISA_NVM_SLOTINFO_SIZE);

		for (f = 0; f < (int)slotp->functions; f++, funcp++) {
			if (eisa_parse_func(
			    (void *)funcp, (void *)str, stype) == CA_SUCCESS)
				return B_TRUE;
		}
		break;

	case EISA_FUNC_DATA:
		funcp = (eisa_nvm_funcinfo_t *)data;
		if (eisa_parse_func(
		    (void *)funcp, (void *)str, stype) == CA_SUCCESS)
			return B_TRUE;
		break;

	default:
		break;
	};

	return B_FALSE;
}


/*
 * int
 * eisa_rom_call(struct regs *regs)
 *
 * Calling/Exit State:
 *	<regs> is a pointer to the regs structure.
 */
int
eisa_rom_call(struct regs *reg)
{
	if (eisa_bios_vaddr == NULL && eisa_init_bios_entry() != 0)
		return -1;

	ASSERT(eisa_bios_vaddr);

	return(_eisa_rom_call(reg, eisa_bios_vaddr));
}
