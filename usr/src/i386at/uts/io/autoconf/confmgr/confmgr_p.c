#ident	"@(#)kern-i386at:io/autoconf/confmgr/confmgr_p.c	1.54.20.6"
#ident	"$Header$"

/*
 * Autoconfig -- CM/CA Interface
 *
 * Architecture or bus-specific section of Configuration Manager.
 *
 * TODO:
 *	Handle failures (If rm_addval fails, then undo the previous addvals).
 *
 * DONE:
 *	Initialize the resource attributes (e.g. shared interrupts).
 *
 *	If on an EISA system with an ISA card, the irq of an EISA card
 *	is changed such that it is in conflict with the ISA card, the
 *	cm_init must have the intelligence to flag the DCU/user to
 *	run the ECU/PCU to reconfigure the EISA card or rejumper the
 *	ISA card.
 *
 * 	Initialized multi-valued resource parameters.
 *
 *	Multiple instances of the same board. A mapping table between
 *	the <rm_key_t> and the <device_id -- bus_access> has to be
 *	maintained to resolve the matching board id conflict.
 *
 *	Multiple resource entries per slot. For example, a system
 *	board or integrated controllers contains resource description
 *	of multiple devices.
 *
 */
#define	_DDI_C

#include <svc/systm.h>
#include <io/autoconf/ca/ca.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <mem/kmem.h>
#include <svc/errno.h>
#include <svc/eisa.h>
#include <svc/psm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/mod/moddrv.h>
#include <util/param.h>
#include <util/types.h>
#include <proc/cg.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/ddi.h>

/* DO WE NEED/WANT ddi.h ?? */

/* For debugging only */
#undef STATIC
#define STATIC

#if defined(DEBUG) || defined(DEBUG_TOOLS)
int cm_debug = 0;
#define DEBUG1(a)	if (cm_debug == 1) printf a
#define DEBUG2(a)	if (cm_debug == 2) printf a
#else
#define DEBUG1(a)
#define DEBUG2(a)
#endif /* DEBUG  || DEBUG_TOOLS */

/*
 * MACRO
 * SET_RM_ARGS(struct rm_args *rma, rm_key_t key, 
 *				void *val, int vlen, int n)
 *	Set add data fields of <rma>.
 *
 * Calling/Exit State:
 *	None.
 */
#define	SET_RM_ARGS(rma, param, key, val, vlen, n) { \
	(void)strcpy((rma)->rm_param, (param)); \
	(rma)->rm_key = (key); \
	(rma)->rm_val = (val); \
	(rma)->rm_vallen = (vlen); \
	(rma)->rm_n = (n); \
} 

/* Offsets into the array _ca_rw[] */

#define _CM_READ_8	0
#define _CM_READ_16	1
#define _CM_READ_32	2
#define _CM_WRITE_8	3
#define _CM_WRITE_16	4
#define _CM_WRITE_32	5

extern int	ca_read_devconfig8(),
		ca_read_devconfig16(),
		ca_read_devconfig32(),
		ca_write_devconfig8(),
		ca_write_devconfig16(),
		ca_write_devconfig32();

int (*_ca_rw[])() = {

	ca_read_devconfig8,
	ca_read_devconfig16,
	ca_read_devconfig32,
	ca_write_devconfig8,
	ca_write_devconfig16,
	ca_write_devconfig32
};

#define	CM_NUM_SIZE		(sizeof(cm_num_t))
#define	CM_ADDR_RNG_SIZE	(sizeof(struct cm_addr_rng))

#define EISA_SLOT(x)	((x) & 0x0FF)
#define MCA_SLOT(x)		((x) & 0x0FF)

/*
 * CM_MAXHEXDIGITS is max length of 64-bit ulong_t converted to a
 * HEX ASCII string with 0x prepended.
 */

#define CM_MAXHEXDIGITS		19
#define CM_HEXBASE		16

struct cm_key_list
{
	rm_key_t		key;
	char			brdid[CM_MAXHEXDIGITS];
	struct cm_key_list	*knext;
};

struct cm_cip_list
{
	struct config_info	*cip;
	char			brdid[CM_MAXHEXDIGITS];
	int			order;
	struct cm_cip_list	*cnext;
};

uint_t				_cm_bustypes = 0;

STATIC struct cm_cip_list	*_cm_new_cips = NULL;
STATIC struct cm_key_list	*_cm_inuse_keys = NULL;
STATIC struct cm_key_list	*_cm_purge_keys = NULL;
STATIC struct cm_key_list	*_cm_boot_hba = NULL;

STATIC cm_num_t _cm_get_brdbustype(rm_key_t key);
STATIC cm_num_t _cm_get_brddevfunc(rm_key_t key);
STATIC cm_num_t _cm_get_brdfunc(rm_key_t key);
STATIC cm_num_t _cm_get_brdslot(rm_key_t key);

extern int		cm_find_match(struct rm_args *, void *, size_t);
extern void		_cm_save_cip(struct config_info * );
extern void		_cm_match_em(struct cm_key_list **, struct cm_cip_list **);
extern void		_cm_add_vals(rm_key_t, struct config_info *, boolean_t);
extern int		cm_add_vals(rm_key_t, struct config_info *);
extern void		_cm_add_entry(struct cm_cip_list *, rm_key_t);
extern int		cm_add_entry(struct config_info *, rm_key_t);
extern void		_cm_update_key(struct cm_cip_list *, struct cm_key_list *);
extern void		_cm_sync_up(void);
extern void		_cm_itoh(ulong_t, char[], int);
extern boolean_t	_cm_chk_key(rm_key_t);
extern void		_cm_save_key(rm_key_t, struct cm_key_list **, boolean_t);
extern void		_cm_clean_klist(void);
extern int		_cm_strncmp(const char *, const char *, int);

char	*cm_bootarg[10];
uint_t	cm_bootarg_count;
uint_t	cm_bootarg_max = sizeof(cm_bootarg) / sizeof(cm_bootarg[0]);


/*
 * STATIC void
 * _cm_bootarg_parse(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Parse boot arguments and add/delete/replace the parameters.
 *	Currently, we only care about arguments that have the
 *	following form:
 *
 *		<modname>:[<instance>:]<param>=<value>
 */
STATIC void
_cm_bootarg_parse(void)
{
	int	i;			/* index */
	int	rv;			/* return value */
	char	*bas;			/* boot arg string */
	char	*baid;			/* boot arg id (driver modname) */
	char	*baparam;		/* boot arg parameter */
	char	*baval;			/* boot arg parameter value */
	char	val[RM_MAXPARAMLEN];	/* max storage for parameter value */
	char	*s;			/* temporary ptr to the boot string */
	struct rm_args rma;		/* packet to pass to resmgr */
	rm_key_t key;			/* modname, instance key */
	int	inst;			/* board instance */

	for (i = 0; i < cm_bootarg_count; i++) {

		inst = 0;
		baid = cm_bootarg[i];
		s = strpbrk(baid, ":=");
		if (s == NULL || *s != ':')
			continue;
		*s++ = '\0';
		bas = s;
		s = strpbrk(bas, ":=");
		if (s == NULL)
			continue;
		if (*s == ':') {
			*s++ = '\0';
			inst = stoi(&bas);
			bas = s;
			s = strpbrk(bas, ":=");
			if (s == NULL || *s != '=')
				continue;
		}
		baparam = bas;
		*s++ = '\0';
		baval = s;
	
		DEBUG1(("baparam=%s, baid=%s, inst=%d, baval=%s\n",
				baparam, baid, inst, baval));

		if ((key = _Compat_cm_getbrdkey(baid, inst)) != RM_NULL_KEY) {
			SET_RM_ARGS(&rma, baparam, key, val, RM_MAXPARAMLEN, 0);
			switch ((rv = rm_getval(&rma, UIO_SYSSPACE))) { 
			case 0:
				if (strcmp(baval, val) == 0)
					continue;
				/* Replace (delete and add) existing value. */
				(void) rm_delval(&rma);
				/* FALLTHROUGH */
			case ENOENT:
				rma.rm_vallen = strlen(baval) + 1;
				rma.rm_val = baval;
				(void) rm_addval(&rma, UIO_SYSSPACE);
				break;
			default:
				ASSERT(rv != ENOSPC);
				break;
			};
		}
	}
}

/*
 * int
 * cm_init_p(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
cm_init_p(void)
{
	struct rm_args		rma;
	cm_num_t		dcu_mode = CM_DCU_NOCHG;
	cm_num_t		bustype;
	time_t			rm_time = 1;
	int			error;

	error = ca_init();

	/*
	** The bits for the other buses are set in ca_init().  I'm
	** NOT crazy about setting _cm_bustypes directly in ca, but
	** this is a temporary interface and I'm striving for
	** simplicity.
	**
	** The basic algorithm to set the ISA bit is to see if we
	** can positively determine if there's an ISA bus, if we
	** can't determine the eixstance of an ISA bus we assume
	** ISA if ! MCA (this will work for all except an MCA/ISA
	** system, theoretically possible, but highly unlikely.)
	**
	** BUT .... for now, I'm just going with ! MCA.
	*/

	if (( _cm_bustypes & CM_BUS_MCA ) == 0 )
		_cm_bustypes |= CM_BUS_ISA;

	/*
	** Save value in resmgr to give user space access to it.
	**
	** No need to check for error from rm_addval, nothing I can do.
	*/

	rma.rm_key = RM_KEY;
	(void)strcpy( rma.rm_param, CM_BUSTYPES );
	rma.rm_val = &_cm_bustypes;
	rma.rm_vallen = sizeof( _cm_bustypes );
	(void)_rm_addval( &rma, UIO_SYSSPACE );

	if (_cm_new_cips != NULL) {
		/* We found new entries, but haven't added them yet */
		dcu_mode = CM_DCU_SILENT;
	} else {
		/* Check to see if we've changed the resmgr database */

		(void)strcpy(rma.rm_param, RM_TIMESTAMP);
		rma.rm_val = &rm_time;
		rma.rm_vallen = sizeof(rm_time);
		rma.rm_n = 0;

		/*
		 * This should NEVER fail, but just in case it ever does,
		 * we'll assume the resmgr database has changed.
		 */
		(void)_rm_getval(&rma, UIO_SYSSPACE);

		if (rm_time != 0)
			dcu_mode = CM_DCU_SILENT;
	}

	(void)strcpy(rma.rm_param, CM_DCU_MODE);
	rma.rm_val = &dcu_mode;
	rma.rm_vallen = sizeof(dcu_mode);
	(void)_rm_addval(&rma, UIO_SYSSPACE);

	/*
	 * If there are NO errors, then walk through resmgr looking for
	 * obsolete entries.
	 */

	if (error == 0) {

		(void)strcpy(rma.rm_param, CM_BRDBUSTYPE);
		rma.rm_vallen = sizeof(bustype);
		rma.rm_val = &bustype;
		rma.rm_n = 0;

		rma.rm_key = RM_NULL_KEY;

		while (rm_nextkey(&rma) == 0) {

			/* If we have key in list, then DON'T purge entry */

			if (_cm_chk_key(rma.rm_key) == B_TRUE)
				continue;

			/* Can't purge entry if there's no BRDBUSTYPE */

			if (_rm_getval(&rma, UIO_SYSSPACE) != 0)
				continue;

			switch (bustype) {
			/* We can only purge those with NVRAM */
			case CM_BUS_EISA:
			case CM_BUS_MCA:
			case CM_BUS_PCI:
				_cm_save_key(rma.rm_key, &_cm_purge_keys, B_TRUE);
				break;
			default:
				break;
			}
		}
	}

	_cm_sync_up();

	_cm_clean_klist();

	_cm_bootarg_parse();
}

/*
 * rm_key_t
 * cm_findkey(struct config_info *cip)
 *	Find an existing entry in the resmgr in-core database with a matching 
 *	<board id, bus type, bus access> tuple.
 *
 * Calling/Exit State: None
 */

STATIC rm_key_t
cm_findkey(struct config_info *cip)
{
	struct rm_args	id_rma;			/* brdid access rm_args */
	struct rm_args	drma;			/* dev/bus access rm_args */
	struct rm_args	brma;			/* bus type rm_args */
	cm_num_t	ba;			/* bus/device access */
	char		id[CM_MAXHEXDIGITS];	/* board id */
	cm_num_t	btype;			/* type of bus */
	cm_num_t	dba;			/* bus/device access */
	cm_num_t	bustype;		/* type of bus */
	char		bid[CM_MAXHEXDIGITS];	/* board id */
	char		*brdid = bid;

	extern boolean_t ca_eisa_clone_slot( ulong_t, ulong_t );

	bustype = cip->ci_busid;

	switch (bustype) {
	case CM_BUS_EISA:
		brdid = eisa_uncompress((char *)&cip->ci_eisabrdid);
		dba = (cm_num_t)cip->ci_eisaba;
		break;

	case CM_BUS_MCA:
		_cm_itoh(cip->ci_mcabrdid, brdid, 4);
		dba = (cm_num_t)cip->ci_mcaba;
		break;

	case CM_BUS_PCI:
		_cm_itoh(cip->ci_pcibrdid, brdid, 0);
		dba = (cm_num_t)cip->ci_pciba;
		break;

	default:
		return RM_NULL_KEY;
	}

	SET_RM_ARGS(&id_rma, CM_BRDID, RM_NULL_KEY, id, sizeof(bid), 0); 
	SET_RM_ARGS(&brma, CM_BRDBUSTYPE, RM_NULL_KEY, &btype, CM_NUM_SIZE, 0);
	SET_RM_ARGS(&drma, CM_CA_DEVCONFIG, RM_NULL_KEY, &ba, CM_NUM_SIZE, 0);

	/*
	 * Note: The consecutive cm_find_match begins the search from
	 * the last brdid match.
	 */

	while (cm_find_match(&id_rma, brdid, strlen(brdid) + 1) == 0) {
		/* Check for bustype match */

		brma.rm_key = id_rma.rm_key;

		if (_rm_getval(&brma, UIO_SYSSPACE) != 0 || btype != bustype)
			continue;

		/* Check for bus access match */

		drma.rm_key = id_rma.rm_key;

		if (_rm_getval(&drma, UIO_SYSSPACE) != 0)
			continue;

		if (ba == dba || (btype == CM_BUS_EISA  &&
		    ca_eisa_clone_slot((ulong_t)ba, (ulong_t)dba) == B_TRUE))
			return id_rma.rm_key;
	}

	return RM_NULL_KEY;
}

/*
 * STATIC int
 * _cm_check_irq(struct rm_args *rma, struct config_info *cip)
 *
 * Calling/Exit State:
 *	Return 0 on success, and 1 on failure.
 */
STATIC int
_cm_check_irq(struct rm_args *rma, struct config_info *cip)
{
	cm_num_t	irq;
	int		ret = 0;

	rma->rm_val = &irq;
	rma->rm_n = 0;

	while (_rm_getval( rma, UIO_SYSSPACE) == 0) {
		if (irq != (cm_num_t)cip->ci_irqline[ rma->rm_n++]) {
			ret = 1;
			break;
		}
	}

	if (ret == 1 || rma->rm_n != (uint_t)cip->ci_numirqs) {
		(void)_rm_delval(rma);
		return 1;
	}

	return ret;
}

/*
 * STATIC int
 * _cm_check_itype(struct rm_args *rma, cm_num_t itype[], int count)
 *
 * Calling/Exit State:
 *	Return 0 on success, and 1 on failure.
 */
STATIC int
_cm_check_itype(struct rm_args *rma, cm_num_t itype[], int count)
{
	cm_num_t	val;
	int		ret = 0;

	rma->rm_val = &val;
	rma->rm_n = 0;

	while (_rm_getval( rma, UIO_SYSSPACE) == 0) {
		if (itype[rma->rm_n] != 0 && val != itype[rma->rm_n]) {
			ret = 1;
			break;
		}
		rma->rm_n++;
	}

	if (ret == 1 || rma->rm_n != count) {
		(void)_rm_delval( rma );
		return 1;
	}

	return ret;
}

/*
 * STATIC int
 * _cm_check_dma(struct rm_args *rma, struct config_info *cip)
 *
 * Calling/Exit State:
 *	Return 0 on success, and 1 on failure.
 */
STATIC int
_cm_check_dma(struct rm_args *rma, struct config_info *cip)
{
	cm_num_t	dma;
	int		ret = 0;

	rma->rm_val = &dma;
	rma->rm_n = 0;

	while (_rm_getval(rma, UIO_SYSSPACE) == 0) {
		if (dma != (cm_num_t)cip->ci_dmachan[rma->rm_n++]) {
			ret = 1;
			break;
		}
	}

	if (ret == 1 || rma->rm_n != (uint_t)cip->ci_numdmas) {
		(void)_rm_delval( rma );
		return 1;
	}

	return ret;
}

/*
 * int
 * _cm_check_ioaddr(struct rm_args *rma, struct config_info *cip)
 *
 * Calling/Exit State:
 *	Return 0 on success, and 1 on failure.
 */
STATIC int
_cm_check_ioaddr(struct rm_args *rma, struct config_info *cip)
{
	struct cm_addr_rng	addr_rng;
	int			ret = 0;
	int			adj;

	rma->rm_val = &addr_rng;
	rma->rm_n = 0;

	while (_rm_getval(rma, UIO_SYSSPACE) == 0) {

		adj = 0;

		if (cip->ci_ioport_length[rma->rm_n] != 0)
                        adj = 1;

		if (addr_rng.startaddr !=
				(long)cip->ci_ioport_base[rma->rm_n] ||
		     addr_rng.endaddr != addr_rng.startaddr +
				(long) cip->ci_ioport_length[rma->rm_n] - adj) {
			rma->rm_n++;
			ret = 1;
			break;
		}

		rma->rm_n++;
	}

	if (ret == 1 ||  rma->rm_n != (uint_t)cip->ci_numioports) {
		(void)_rm_delval(rma);
		return 1;
	}

	return ret;
}

/*
 * int
 * _cm_check_memaddr(struct rm_args *rma, struct config_info *cip)
 *
 * Calling/Exit State:
 *	Return 0 on success, and 1 on failure.
 */
STATIC int
_cm_check_memaddr(struct rm_args *rma, struct config_info *cip)
{
	struct cm_addr_rng	addr_rng;
	int			ret = 0;

	rma->rm_val = &addr_rng;
	rma->rm_n = 0;

	while (_rm_getval(rma, UIO_SYSSPACE) == 0) {
		if (addr_rng.startaddr !=
				(long)cip->ci_membase[rma->rm_n] ||
		     addr_rng.endaddr != addr_rng.startaddr +
				(long) cip->ci_memlength[rma->rm_n] - 1) {
			rma->rm_n++;
			ret = 1;
			break;
		}

		rma->rm_n++;
	}

	if (ret == 1 || rma->rm_n != (uint_t)cip->ci_nummemwindows) {
		(void)_rm_delval(rma);
		return 1;
	}

	return ret;
}


/*
 * int
 * cm_register_devconfig(struct config_info *cip)
 *	Register the device configuration information with the resource mgr.
 *
 * Calling/Exit State:
 *	None.
 */
int
cm_register_devconfig(struct config_info *cip)
{
	rm_key_t	key;

	if ((key = cm_findkey( cip )) == RM_NULL_KEY) {
		/*
		 * This is a new entry in NVRAM, save the information
		 * for later consumption.
		 */
		_cm_save_cip( cip );
		return 0;
	}

	DEBUG1(("cm_register_devconfig: key=0x%x\n", key));

	/*
	 * Save the keys as we deal with them.  Then in cm_init_p()
	 * we'll use this list of keys when we walk through the resmgr
	 * database and purge those entries that are no longer needed.
	 */

	_cm_save_key(key, &_cm_inuse_keys, B_FALSE);

	/* Since we can't interpret NVRAM info on MCA systems--we're done */

	if (cip->ci_busid == CM_BUS_MCA)
		return 0;

	_cm_add_vals( key, cip, B_FALSE );
}

STATIC void
_cm_add_vals( rm_key_t key, struct config_info *cip, boolean_t newkey )
{
	struct config_irq_info	*cirqip;
	struct cm_addr_rng	ioportrng;
	struct cm_addr_rng	memrng;
	struct rm_args		rma;
	cm_num_t		itype_val;
	cm_num_t		itype[MAX_IRQS];
	cm_num_t		claim = 0;
	cm_num_t		irq;
	cm_num_t		dma;
	int			i;

	rma.rm_key = key;
	rma.rm_n = 0;

	if (newkey == B_FALSE) {
		(void)strcpy(rma.rm_param, CM_CLAIM);
		rma.rm_vallen = sizeof(claim);
		rma.rm_val = &claim;

		(void)_rm_getval(&rma, UIO_SYSSPACE);
	}

	/*
	 * Register IRQ.
	 */ 

	rma.rm_vallen = sizeof(irq);
	(void)strcpy(rma.rm_param, CM_IRQ);

	if (newkey == B_TRUE || (claim & CM_SET_IRQ) == 0  &&
					_cm_check_irq(&rma, cip) != 0) {
		rma.rm_val = &irq;

		for (i = 0; i < (int)cip->ci_numirqs; i++) {
			irq = (cm_num_t)cip->ci_irqline[i];

			if (_rm_addval(&rma, UIO_SYSSPACE) != 0) 
				cmn_err(CE_NOTE,
					"Could not add %s param", rma.rm_param);
		}
	}

	/*
	 * Register ITYPE (interrupt type -- edge/level shared/!shared.
	 */

	for (i = 0; i < (int)cip->ci_numirqs; i++) {
		cirqip = (struct config_irq_info *)&cip->ci_irqattrib[i];

		if (cirqip->cirqi_trigger) {
			/*
			 * This controller uses a level-sensitive
			 * interrupt vector, which can be shared with
			 * any controller for any driver.
			 */
			itype[i] = 4;
		} else if (!cirqip->cirqi_trigger && cirqip->cirqi_type) {
			/*
			 * This controller uses an interrupt vector which
			 * can be shared with any controller for any driver.
			 */
			itype[i] = 3;
		} else if (!cirqip->cirqi_trigger && !cirqip->cirqi_type) {
			/*
			 * This controller uses an interrupt vector which
			 * is not sharable, even with another controller
			 * for the same driver.
			 */
			itype[i] = 1;
		} else {
			/*
			 * No interrupt type information available.
			 */
			itype[i] = 0;
		}
	}

	rma.rm_vallen = sizeof(itype_val);
	(void)strcpy(rma.rm_param, CM_ITYPE);

	if (newkey == B_TRUE || (claim & CM_SET_ITYPE) == 0  &&
			_cm_check_itype(&rma, itype, cip->ci_numirqs) != 0) {
		rma.rm_val = &itype_val;

		for (i = 0; i < (int)cip->ci_numirqs; i++) {
			itype_val = itype[i];
			if (itype_val == 0)
				continue;

			if (_rm_addval(&rma, UIO_SYSSPACE) != 0) 
				cmn_err(CE_NOTE,
					"Could not add %s param", rma.rm_param);
		}
	}

	/*
	 * Register DMA channel.
	 */

	rma.rm_vallen = sizeof(dma);
	(void)strcpy(rma.rm_param, CM_DMAC);

	if (newkey == B_TRUE || (claim & CM_SET_DMAC) == 0 &&
					_cm_check_dma(&rma, cip) != 0) {
		rma.rm_val = &dma;

		for (i = 0; i < (int)cip->ci_numdmas; i++) {
			dma = (cm_num_t)cip->ci_dmachan[i];

			if (_rm_addval(&rma, UIO_SYSSPACE) != 0) 
				cmn_err(CE_NOTE,
					"Could not add %s param", rma.rm_param);
		}
	}

	/*
	 * Register I/O port.
	 */

	rma.rm_vallen = sizeof(ioportrng);
	(void)strcpy(rma.rm_param, CM_IOADDR);

	if (newkey == B_TRUE || (claim & CM_SET_IOADDR) == 0 &&
					_cm_check_ioaddr(&rma, cip) != 0) {
		rma.rm_val = &ioportrng;

		for (i = 0; i < (int)cip->ci_numioports; i++) {
			ioportrng.startaddr = (long) cip->ci_ioport_base[i];
			if (cip->ci_ioport_length[i] == 0)
				ioportrng.endaddr = ioportrng.startaddr;
			else if (cip->ci_ioport_length[i] > 0)
				ioportrng.endaddr = ioportrng.startaddr +
					(long) cip->ci_ioport_length[i] - 1;

			if (_rm_addval(&rma, UIO_SYSSPACE) != 0) 
				cmn_err(CE_NOTE,
					"Could not add %s param", rma.rm_param);
		}
	}

	/*
	 * Register Memory Address Range.
	 */

	rma.rm_vallen = sizeof(memrng);
	(void)strcpy(rma.rm_param, CM_MEMADDR);

	if (newkey == B_TRUE || (claim & CM_SET_MEMADDR) == 0 &&
					_cm_check_memaddr(&rma, cip) != 0) {
		rma.rm_val = &memrng;

		for (i = 0; i < (int)cip->ci_nummemwindows; i++) {
			memrng.startaddr = (long) cip->ci_membase[i];
			memrng.endaddr = memrng.startaddr +
					(long) cip->ci_memlength[i] - 1;

			if (_rm_addval(&rma, UIO_SYSSPACE) != 0) 
				cmn_err(CE_NOTE,
					"Could not add %s param", rma.rm_param);
		}
	}
}

STATIC boolean_t
_cm_chk_key( rm_key_t key )
{
	struct cm_key_list	*keyp = _cm_inuse_keys;

	while ( keyp != NULL  &&  keyp->key != key )
		keyp = keyp->knext;

	if ( keyp == NULL )
		return B_FALSE;

	return B_TRUE;

	/* return keyp != NULL; ?? */
}

STATIC void
_cm_save_key( rm_key_t key, struct cm_key_list **klist, boolean_t purgelist )
{
	struct cm_key_list	*keyp;
	struct cm_key_list	**addit = klist;
	struct rm_args		rma;
	cm_num_t		boothba;

	/*
	** This needs to deal with an arbitrary number of keys.
	**
	** Using a linked list, one node per key, seems kinda
	** wasteful, but considering they're aren't going to
	** be that many entries in NVRAM, it's the least
	** complicated way, and I don't think the costs are
	** that high in terms of time and space.
	*/

	keyp = (struct cm_key_list *)kmem_alloc( sizeof( *keyp ), KM_SLEEP );

	keyp->key = key;

	if ( purgelist == B_TRUE )
	{
		(void)strcpy( rma.rm_param, CM_BRDID );
		rma.rm_key = key;
		rma.rm_val = keyp->brdid;
		rma.rm_vallen = CM_MAXHEXDIGITS;
		rma.rm_n = 0;

		if ( _rm_getval( &rma, UIO_SYSSPACE ) != 0 )
			keyp->brdid[ 0 ] = '\0';

		/* NOT needed if brdid[0] = '\0' */

		while ( *addit != NULL )
		{
			if ( _cm_strncmp( keyp->brdid, (*addit)->brdid, CM_MAXHEXDIGITS ) <= 0 )
				break;

			addit = &(*addit)->knext;
		}

		(void)strcpy( rma.rm_param, CM_BOOTHBA );
		rma.rm_val = &boothba;
		rma.rm_vallen = sizeof( boothba );
		rma.rm_n = 0;
		
		if ( _rm_getval( &rma, UIO_SYSSPACE ) == 0 )
		{
			/* Value of param is irrelevant */

			_cm_boot_hba = keyp;
			return;
		}
	}

	keyp->knext = *addit;
	*addit = keyp;
}

STATIC void
_cm_clean_klist( void )
{
	struct cm_key_list	*keyp = _cm_inuse_keys;
	struct cm_key_list	*tmpkeyp;

	while (keyp != NULL) {
		tmpkeyp = keyp->knext;
		kmem_free(keyp, sizeof(*keyp));
		keyp = tmpkeyp;
	}
}

STATIC void
_cm_save_cip( struct config_info *cip )
{
	struct cm_cip_list	**addit = &_cm_new_cips;
	struct cm_cip_list	*newcip;
	char			bid[ CM_MAXHEXDIGITS ];
	char			*brdid = bid;
	static int		order = 1;

	switch (cip->ci_busid) {
	case CM_BUS_EISA:
		brdid = eisa_uncompress((char *)&cip->ci_eisabrdid);
		break;

	case CM_BUS_MCA:
		_cm_itoh(cip->ci_mcabrdid, bid, 4);
		break;

	case CM_BUS_PCI:
		_cm_itoh(cip->ci_pcibrdid, bid, 0);
		break;

	default:
		return;
	}

	/*
	** This needs to deal with an arbitrary number of entries.
	**
	** Using a linked list, one node per key, seems kinda
	** wasteful, but considering they're aren't going to
	** be that many entries in NVRAM, it's the least
	** complicated way, and I don't think the costs are
	** that high in terms of time and space.
	*/

	newcip = (struct cm_cip_list *)kmem_alloc(sizeof(*newcip), KM_SLEEP);

	(void)strncpy(newcip->brdid, brdid, CM_MAXHEXDIGITS);

	/*
	** In addition to keeping the list in sorted order, based on BRDID,
	** I'm going to keep track of the order I received them from CA.
	** Then when I'm done syncing up the list with the obsolete entries
	** from the resource manager, I'll add the new entries in the same
	** order I received them from CA.  This will preserve adding the
	** entries in "slot order."
	*/

	newcip->order = order++;

	/* Now insert in sorted order, based on brdid */

	while (*addit != NULL) {
		if (_cm_strncmp(brdid, (*addit)->brdid, CM_MAXHEXDIGITS) <= 0)
			break;

		addit = &(*addit)->cnext;
	}

	newcip->cip = cip;
	newcip->cnext = *addit;
	*addit = newcip;
}

STATIC int
_cm_strncmp( const char *str1, const char *str2, int max )
{
	while ( max-- > 0  &&  *str1 != '\0'  &&  *str1 == *str2 )
	{
		str1++;
		str2++;
	}

	if ( max < 0 )
		return 0;

	return *str1 - *str2;
}

STATIC void
_cm_add_entry( struct cm_cip_list *clist, rm_key_t key )
{
	struct config_info	*cip = clist->cip;
	struct rm_args		rma;
	cm_num_t		slot = -1;
	cm_num_t		ba;			/* bus/device access */
	cm_num_t		bustype;		/* type of bus */
	cm_num_t		devnum;
	cm_num_t		funcnum = -1;
	cm_num_t		busnum;
	ms_cgid_t		cgid;

	char			sbrdid[CM_MAXHEXDIGITS];

	bustype = cip->ci_busid;

	switch (bustype) {
	case CM_BUS_EISA:
		ba = (cm_num_t)cip->ci_eisaba;
		slot = EISA_SLOT(ba);
		funcnum = cip->ci_eisa_funcnumber;
		break;

	case CM_BUS_MCA:
		ba = (cm_num_t)cip->ci_mcaba;
		slot = MCA_SLOT(ba);
		break;

	case CM_BUS_PCI:
		ba = (cm_num_t)cip->ci_pciba;
		slot = (cm_num_t) cip->ci_pcislot;
		funcnum = cip->ci_pci_devfuncnumber & 0x7;
		break;

	default:
		return;
	}

	/*
	 * Get a new key and register all the system resources.
	 */

	if (key != RM_NULL_KEY)
		rma.rm_key = key;
	else if (_rm_newkey(&rma) != 0){
		return;
	}

	/* Register bus type */

	(void)strcpy( rma.rm_param, CM_BRDBUSTYPE );
	rma.rm_vallen = sizeof(bustype);
	rma.rm_val = &bustype;

	if (_rm_addval(&rma, UIO_SYSSPACE) != 0) {
		(void)_rm_delkey(&rma);
		return;
	}

	/* Register device board id */

	(void)strcpy(rma.rm_param, CM_BRDID);
	rma.rm_vallen = strlen( clist->brdid ) + 1;
	rma.rm_val = clist->brdid;

	if (_rm_addval(&rma, UIO_SYSSPACE) != 0) {
		(void)_rm_delkey(&rma);
		return;
	}

	/*
	 * Register device specific information necessary to read/write
	 * the configuration space.
	 */

	(void)strcpy(rma.rm_param, CM_CA_DEVCONFIG);
	rma.rm_vallen = sizeof(ba);
	rma.rm_val = &ba;

	if (_rm_addval(&rma, UIO_SYSSPACE) != 0) {
		(void)_rm_delkey(&rma);
		return;
	}

	/* Register slot information */

	if (slot != -1) {
		(void)strcpy(rma.rm_param, CM_SLOT);
		rma.rm_vallen = sizeof(slot);
		rma.rm_val = &slot;

		if (_rm_addval(&rma, UIO_SYSSPACE) != 0) {
			(void)_rm_delkey(&rma);
			return;
		}
	}

	/* Register function number */

	if ( funcnum != -1 )
	{
		(void)strcpy(rma.rm_param, CM_FUNCNUM);
		rma.rm_vallen = sizeof(funcnum);
		rma.rm_val = &funcnum;

		if (_rm_addval(&rma, UIO_SYSSPACE) != 0) {
			(void)_rm_delkey(&rma);
			return;
		}
	}

	/* Since we can't interpret NVRAM info on MCA systems--we're done */

	if (cip->ci_busid == CM_BUS_MCA)
		return;

	/* Add PCI bus specific params */

	if (cip->ci_busid == CM_BUS_PCI)
	{
		/* C guarantees NO sign extend */

		devnum = (cip->ci_pci_devfuncnumber >> 3);

		(void)strcpy(rma.rm_param, CM_DEVNUM);
		rma.rm_vallen = sizeof(devnum);
		rma.rm_val = &devnum;

		if (_rm_addval(&rma, UIO_SYSSPACE) != 0) {
			(void)_rm_delkey(&rma);
			return;
		}

		busnum = cip->ci_pci_busnumber;

		(void)strcpy(rma.rm_param, CM_BUSNUM);
		rma.rm_vallen = sizeof(busnum);
		rma.rm_val = &busnum;

		if (_rm_addval(&rma, UIO_SYSSPACE) != 0) {
			(void)_rm_delkey(&rma);
			return;
		}

		/*
 		 * PCI devices are the only ccNUMA-capable devices.
		 * Blast away any existing CGID parameter for the
		 * device, and only add one back if we can positively
		 * determine a valid CGID for the device's CG number
		 * (something that only ccNUMA PSMs should be capable
		 * of).  This handles cases such as using an ATup PSM
		 * on a NUMA-hardware system's initial boot, before
		 * building and booting a ccNUMA PSM kernel for the
		 * system.
		 */
		(void)strcpy(rma.rm_param, CM_CGID);
		(void)_rm_delval(&rma);

		cgid = cgnum2cgid(cip->ci_pci_cgnum);

		(void)strcpy(rma.rm_param, CM_CGID);
		rma.rm_vallen = sizeof(cgid);
		rma.rm_val = &cgid;

		if (_rm_addval(&rma, UIO_SYSSPACE) != 0) {
			(void)_rm_delkey(&rma);
			return;
		}

		if (cip->ci_pcisbrdid != 0) { /* ul95-33214 */
			_cm_itoh(cip->ci_pcisbrdid, sbrdid, 0);
			(void)strcpy(rma.rm_param, CM_SBRDID);
			rma.rm_vallen = strlen(sbrdid) + 1;
			rma.rm_val = &sbrdid;

			if (_rm_addval(&rma, UIO_SYSSPACE) != 0) {
				(void)_rm_delkey(&rma);
				return;
			}
		}

		if (cip->ci_pciclassid != 0) {
			_cm_itoh(cip->ci_pciclassid, sbrdid, 4);
			(void)strcpy(rma.rm_param, CM_SCLASSID);
			rma.rm_vallen = strlen(sbrdid) + 1;
			rma.rm_val = &sbrdid;

			if (_rm_addval(&rma, UIO_SYSSPACE) != 0) {
				(void)_rm_delkey(&rma);
				return;
			}
		}
	}

	_cm_add_vals( rma.rm_key, cip, key == RM_NULL_KEY ? B_TRUE : B_FALSE );
}

/*
 * This routine is used to try and improve our method of searching
 * for a new boot controller.
 *
 * As a minimum check, we make sure that the board ID, bus type (PCI etc.)
 * and function number (where applicable) are the same.
 *
 * If the 'exact' flag is set then this routine takes slot number into
 * account as well as other attributes, where this information is available.
 * If a card is found that matches this stricter check then we can probably
 * assume that our boot device has been left in the same slot...
 */
STATIC int
_cm_boardmatch( struct cm_cip_list *clistp, struct cm_key_list *klistp,
		int exact)
{
	int     match = 0;
	int	ret;
	int     slot, oslot;
	int     devfunc, odevfunc;

	if ((ret = _cm_strncmp( clistp->brdid, klistp->brdid, CM_MAXHEXDIGITS )) != 0)
		return ret;
	DEBUG1(("board match key = %d brdid = \n", klistp->key, klistp->brdid));
	if (clistp->cip->ci_busid != _cm_get_brdbustype(klistp->key))
		goto out;

	/*
	 * If we got this far then we have a board ID and bustype that
	 * match. Now we check the function and slot numbers too.
	 */
	DEBUG1(("busid match %x\n", clistp->cip->ci_busid));

	switch (clistp->cip->ci_busid) {
	    case CM_BUS_EISA:
		devfunc = (int)clistp->cip->ci_eisa_funcnumber;
		odevfunc = (int)_cm_get_brdfunc(klistp->key);
		if (exact && (devfunc == odevfunc)) {
			slot = (int)EISA_SLOT((cm_num_t)clistp->cip->ci_eisaba);
			oslot = (int)_cm_get_brdslot(klistp->key);
			if ((slot >= 0) && (oslot >= 0) && (slot == oslot))
				match = 1;
		} else if (devfunc == odevfunc) {
			match = 1;
		}
		break;

	    case CM_BUS_MCA:
		if (exact) {
			slot = (int)MCA_SLOT((cm_num_t)clistp->cip->ci_mcaba);
			oslot = (int)_cm_get_brdslot(klistp->key);
			if ((slot >= 0) && (oslot >= 0) && (slot == oslot))
				match = 1;
		} else {
			match = 1;
		}
		break;

	    case CM_BUS_PCI:
		DEBUG1(("pci bus \n"));
		devfunc = (int)clistp->cip->ci_pci_devfuncnumber;
		odevfunc = (int)_cm_get_brddevfunc(klistp->key);
		if (exact) {
			slot = (int)clistp->cip->ci_pcislot;
			oslot = (int)_cm_get_brdslot(klistp->key);
			if ((devfunc == odevfunc) &&
				(slot >= 0) && (oslot >= 0) && (slot == oslot))
				match = 1;
		} else if ((devfunc & 0x7) == (odevfunc & 0x7)) {
			match = 1;
		}
		break;

	    default:
		cmn_err(CE_WARN, "invalid bus type in _cm_boardmatch()\n");
		break;
	}

    out:
	return match;
}

STATIC void
_cm_sync_up( void )
{
	struct cm_cip_list	*exactmatch;
	struct cm_cip_list	*bestmatch;
	struct cm_cip_list	**clean;
	struct cm_cip_list	*tcips;
	struct cm_key_list	*tkeys;
	struct rm_args		rma;
	cm_range_t		memaddr;
	unsigned long		lowestaddr;
	unsigned long		tlow;
	int			lcv;
	int			cmp;
	int			i;

	/* cases

	simple: - I move a non-boot board for static driver
		- I move a non-boot board for loadable driver
		- I move my boot hba

		- swap positions of two boards
		- shift boards from 2/3 to 3/4
		- with brds in 2/3, move 2 to 4

		advantages of reusing entries--other params maintained
		disadvantages--other params maintained %^)

		We still have the 1540/1740 verify deliema here.  How
		about if I "mark" these using the CM_TYPE or CM_ENTRYTYPE
		params so dcu can double check my work.
	*/

	/* First deal with the case of the boot hba simply moved */
		/* Don't even put this entry into sorted list */

	if ( _cm_boot_hba != NULL )
	{
		bestmatch = NULL;
		lowestaddr = ULONG_MAX;

		tcips = _cm_new_cips;

		while ( tcips != NULL )
		{
			cmp = _cm_boardmatch ( tcips, _cm_boot_hba, 1 );
			if (cmp == 0) {
				exactmatch = tcips;
				break;
			}
			cmp = _cm_boardmatch ( tcips, _cm_boot_hba, 0 );
#ifdef NOMORE
			cmp = _cm_strncmp( tcips->brdid, _cm_boot_hba->brdid, CM_MAXHEXDIGITS );

#endif
			if ( cmp == 0 )
			{

				/*
				** Since we can't interpret NVRAM info on
				** MCA systems--we're going to take the
				** first entry with matching brdid.
				*/

				if ( tcips->cip->ci_busid == CM_BUS_MCA )
				{
					bestmatch = tcips;
					break;
				}

				/* MUST deal with multiple memaddrs */

				if (( lcv = tcips->cip->ci_nummemwindows ) == 0 )
				{
					/*
					** This board has no memory. On a
					** PC-AT system, first such board
					** should be our candidate for the
					** boot controller.
					*/

					bestmatch = tcips;
					break;
				}

				tlow = ULONG_MAX;

				for ( i = 0; i < lcv; i++ )
				{
					if (tcips->cip->ci_membase[ i ] != 0  &&
						tcips->cip->ci_membase[ i ] < tlow )
						tlow = tcips->cip->ci_membase[ i ];
				}

				if ( tlow < lowestaddr )
				{
					lowestaddr = tlow;
					bestmatch = tcips;
				}
			}
			else if ( cmp > 0 )
			{
				break;
			}

			tcips = tcips->cnext;
		}

		if ( exactmatch != NULL )
			bestmatch = exactmatch;

		if ( bestmatch != NULL )
		{
			_cm_update_key( bestmatch, _cm_boot_hba );

			kmem_free( _cm_boot_hba, sizeof( *_cm_boot_hba ));
			_cm_boot_hba = NULL;

			clean = &_cm_new_cips;

			while ( *clean != bestmatch )
				clean = &(*clean)->cnext;

			*clean = (*clean)->cnext;
			kmem_free( bestmatch, sizeof( *bestmatch ));
		}
	}

	/* Now, do I A) check for _cm_boot_hba != NULL again and
	   pick the NEXT best, or B) wait until after I'm done
	   processing the rest of the lists and then try one last
	   time to find a match from what's left ??

	   B for now !!
	*/

	tkeys = _cm_purge_keys;
	tcips = _cm_new_cips;

	while ( tkeys != NULL  &&  tcips != NULL )
	{
		cmp = _cm_strncmp(tkeys->brdid, tcips->brdid, CM_MAXHEXDIGITS);

		if ( cmp == 0 )
			_cm_match_em( &tkeys, &tcips );
		else if ( cmp < 0 )
			tkeys = tkeys->knext;
		else
			tcips = tcips->cnext;
	}

	if ( _cm_boot_hba != NULL  &&  _cm_new_cips != NULL )
	{
	 	 
	}

	/* Now delete remaining obsolete entries */

	while ( _cm_purge_keys != NULL )
	{
		rma.rm_key = _cm_purge_keys->key;
		tkeys = _cm_purge_keys;
		_cm_purge_keys = _cm_purge_keys->knext;
		kmem_free( tkeys, sizeof( *tkeys ));

		(void)_rm_delkey( &rma );
	}

	/*
	** Now add new resmgr entries for the any left in list
	**
	** They will be added in the same relative order as I
	** originally received them from CA to preserve "slot order"
	*/

	for ( ;; )
	{
		tcips = _cm_new_cips;
		cmp = INT_MAX;

		while ( tcips != NULL )
		{
			if ( tcips->order < cmp )
			{
				bestmatch = tcips;
				cmp = tcips->order;
			}

			tcips = tcips->cnext;
		}

		if ( cmp == INT_MAX )
			break;

		_cm_add_entry( bestmatch, RM_NULL_KEY );

		bestmatch->order = INT_MAX;	/* Mark this entry "done" */
	}


	while ( _cm_new_cips != NULL )
	{
		tcips = _cm_new_cips->cnext;
		kmem_free( _cm_new_cips, sizeof( *_cm_new_cips ));
		_cm_new_cips = tcips;
	}
}

STATIC void
_cm_match_em( struct cm_key_list **keysp, struct cm_cip_list **cipsp )
{
	struct cm_key_list	*nextk = (*keysp)->knext;
	struct cm_cip_list	*nextc = (*cipsp)->cnext;
	struct cm_key_list	*tkeys;
	struct cm_cip_list	*tcips;
	boolean_t		lastk = B_FALSE;
	boolean_t		lastc = B_FALSE;
	char			brdid[ CM_MAXHEXDIGITS ];
	
	(void)strncpy( brdid, (*cipsp)->brdid, CM_MAXHEXDIGITS );

	if ( nextk == NULL  ||
		_cm_strncmp( brdid, nextk->brdid, CM_MAXHEXDIGITS ) != 0 )
	{
		lastk = B_TRUE;
	}

	if ( nextc == NULL  ||
		_cm_strncmp( brdid, nextc->brdid, CM_MAXHEXDIGITS ) != 0 )
	{
		lastc = B_TRUE;
	}

	if ( lastc == B_TRUE  &&  lastk == B_TRUE )
	{
		_cm_update_key( *cipsp, *keysp );

		tcips = *cipsp;
		*cipsp = (*cipsp)->cnext;

		cipsp = &_cm_new_cips;

		while ( *cipsp != tcips )
			cipsp = &(*cipsp)->cnext;

		*cipsp = (*cipsp)->cnext;
		kmem_free( tcips, sizeof( *tcips ));

		tkeys = *keysp;
		*keysp = (*keysp)->knext;

		keysp = &_cm_purge_keys;

		while ( *keysp != tkeys )
			keysp = &(*keysp)->knext;

		*keysp = (*keysp)->knext;
		kmem_free( tkeys, sizeof( *tkeys ));
	}
	else
	{
		/*
		** Too complicated to deal with, so bump pointers along
		*/

		*cipsp = nextc;

		while ( *cipsp != NULL  &&
			_cm_strncmp( brdid, (*cipsp)->brdid, CM_MAXHEXDIGITS ) == 0 )
			*cipsp = (*cipsp)->cnext;

		*keysp = nextk;

		while ( *keysp != NULL  &&
			_cm_strncmp( brdid, (*keysp)->brdid, CM_MAXHEXDIGITS ) == 0 )
			*keysp = (*keysp)->knext;
	}
}

STATIC void
_cm_update_key( struct cm_cip_list *cip, struct cm_key_list *key )
{
	struct rm_args	rma;

	rma.rm_key = key->key;

	(void)strcpy( rma.rm_param, CM_CLAIM );
	(void)_rm_delval( &rma );

	(void)strcpy( rma.rm_param, CM_SLOT );
	(void)_rm_delval( &rma );

	(void)strcpy( rma.rm_param, CM_FUNCNUM );
	(void)_rm_delval( &rma );

	(void)strcpy( rma.rm_param, CM_CA_DEVCONFIG );
	(void)_rm_delval( &rma );

	(void)strcpy( rma.rm_param, CM_BRDBUSTYPE );
	(void)_rm_delval( &rma );

	(void)strcpy( rma.rm_param, CM_BRDID );
	(void)_rm_delval( &rma );

	if ( cip->cip->ci_busid == CM_BUS_PCI )
	{
		(void)strcpy( rma.rm_param, CM_BUSNUM );
		(void)_rm_delval( &rma );

		(void)strcpy( rma.rm_param, CM_DEVNUM );
		(void)_rm_delval( &rma );

		(void)strcpy( rma.rm_param, CM_SBRDID );
		(void)_rm_delval( &rma );

		(void)strcpy( rma.rm_param, CM_SCLASSID );
		(void)_rm_delval( &rma );
	}

	_cm_add_entry( cip, key->key );
}

/*
 * STATIC int
 * cm_get_intr_info(rm_key_t key, int *devflagp, struct intr_info *ip)
 *	Utility function to get interrupt info from a key.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
cm_get_intr_info(rm_key_t key, int *devflagp, struct intr_info *ip)
{
	struct rm_args rma;
	cm_num_t irq;
	cm_num_t ipl;
	cm_num_t itype;
	cm_num_t cpu;
	int stat;

	rma.rm_key = key;
	rma.rm_n = 0;

	(void)strcpy(rma.rm_param, CM_IRQ);
	rma.rm_val = &irq;
	rma.rm_vallen = sizeof irq;
	if (_rm_getval(&rma, UIO_SYSSPACE) != 0 || rma.rm_vallen != sizeof irq)
		return -1;
	ip->ivect_no = irq;

	(void)strcpy(rma.rm_param, CM_IPL);
	rma.rm_val = &ipl;
	rma.rm_vallen = sizeof ipl;
	if (_rm_getval(&rma, UIO_SYSSPACE) != 0 || rma.rm_vallen != sizeof ipl)
		return -1;
	ip->int_pri = ipl;

	(void)strcpy(rma.rm_param, CM_ITYPE);
	rma.rm_val = &itype;
	rma.rm_vallen = sizeof itype;
	if (_rm_getval(&rma, UIO_SYSSPACE) != 0 || rma.rm_vallen != sizeof itype)
		return -1;
	ip->itype = itype;

	(void)strcpy(rma.rm_param, CM_BINDCPU);
	rma.rm_val = &cpu;
	rma.rm_vallen = sizeof cpu;
	if ((stat = _rm_getval(&rma, UIO_SYSSPACE)) == ENOENT)
		cpu = -1;
	else if (stat != 0 || rma.rm_vallen != sizeof cpu)
		return -1;
	ip->int_cpu = cpu;

	ip->int_mp = (*devflagp & D_MP);

	return 0;
}

/*
 * Structure used to maintain state between cm_intr_attach and cm_intr_detach.
 */
struct cm_intr_cookie {
	struct intr_info ic_intr_info;
	int		 (*ic_handler)();
};

/*
 * int
 * cm_intr_attach( rm_key_t key, int (*handler)(), void *idata,
 *		   drvinfo_t *infop, void **intr_cookiep )
 *	Attach device interrupts.
 *
 * Calling/Exit State:
 *	May block, so must not be called with basic locks held.
 *
 *	If intr_cookiep is non-NULL, it will be filled in with a cookie
 *	which can be passed to cm_intr_detach to detach the interrupts
 *	when the driver is done with them.  If intr_cookiep is NULL,
 *	the interrupts will never be detached.
 */
int
cm_intr_attach(rm_key_t key, int (*handler)(), void *idata,
	       drvinfo_t *infop, void **intr_cookiep)
{
	struct intr_info ii;

	if (cm_get_intr_info(key, (int *)&infop->drv_flags, &ii) != 0)
		return 0;
	

	if (mod_add_intr(&ii, handler, idata) != 0) {
		if ( intr_cookiep != NULL)
			*intr_cookiep = NULL;
		return 0;
	}
		
	if (intr_cookiep != NULL) {
		*intr_cookiep = kmem_alloc(sizeof(struct cm_intr_cookie),
					   KM_SLEEP);
		ii.int_idata = idata;
		((struct cm_intr_cookie *)*intr_cookiep)->ic_intr_info = ii;
		((struct cm_intr_cookie *)*intr_cookiep)->ic_handler = handler;
	}

	return 1;
}
/*
*
* int
* cm_intr_call(void *intr_cookie)
*
* Calling/exit state:
* 	Used by special base drivers like sysdump to
* 	go between an interrupt cookie, and the actual interrupt handler.
* 	created as part of dev_t cleanup in support of SDI stack becoming ddi8
*
* 	Normal drivers should _never_ call this
*/

int
cm_intr_call(void *intr_cookie) {
	struct cm_intr_cookie *cp = intr_cookie;

	return(cp->ic_handler)(cp->ic_intr_info.int_idata);
}

extern int _Compat_intr_handler(void *);
/*
 * void
 * cm_intr_detach(void *intr_cookie)
 *	Detach device interrupts.
 *
 * Calling/Exit State:
 *	May block, so must not be called with basic locks held.
 *
 *	The intr_cookie must be a value passed back from a previous call
 *	to cm_intr_attach.
 */
void
cm_intr_detach(void *intr_cookie)
{
	struct cm_intr_cookie *icp = intr_cookie;

	if (icp == NULL)
		return;

	(void)mod_remove_intr(&icp->ic_intr_info, icp->ic_handler);
	
	if (icp->ic_handler == &_Compat_intr_handler)
		kmem_free(icp->ic_intr_info.int_idata, sizeof (struct _Compat_intr_idata));

	kmem_free(icp, sizeof *icp);
}


/*
 * STATIC int 
 * _cm_get_ca_devconfig(rm_key_t key, cm_num_t *val)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int 
_cm_get_ca_devconfig(rm_key_t key, cm_num_t *val)
{
	struct rm_args	rma;
	cm_num_t	devconfig;

	SET_RM_ARGS(&rma, CM_CA_DEVCONFIG, key, &devconfig, sizeof(devconfig), 0);
	if (_rm_getval(&rma, UIO_SYSSPACE) != 0 || 
	    rma.rm_vallen != sizeof(devconfig)){
		return -1;
	}

	*val = devconfig;
	return 0;
}

/*
 * STATIC cm_num_t 
 * _cm_get_brdbustype(rm_key_t key)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC cm_num_t
_cm_get_brdbustype(rm_key_t key)
{
	struct rm_args	rma;
	cm_num_t bustype;

	SET_RM_ARGS(&rma, CM_BRDBUSTYPE, key, &bustype, CM_NUM_SIZE, 0);
	if (_rm_getval(&rma, UIO_SYSSPACE) == 0) {
		return bustype;
	}

	return CM_BUS_UNK;
}

/*
 * STATIC cm_num_t
 * _cm_get_brdfunc(rm_key_t key)
 *
 * Calling/Exit State:
 *      None.
 */
STATIC cm_num_t
_cm_get_brdfunc(rm_key_t key)
{
	struct rm_args  rma;
	cm_num_t func;

	SET_RM_ARGS(&rma, CM_FUNCNUM, key, &func, CM_NUM_SIZE, 0);
	if (_rm_getval(&rma, UIO_SYSSPACE) != 0){
		return ( (cm_num_t)(-1) );
	}

	return ( func );
}
/*
 * STATIC cm_num_t
 * _cm_get_brddevfunc(rm_key_t key)
 *
 * Calling/Exit State:
 *      None.
 */
STATIC cm_num_t
_cm_get_brddevfunc(rm_key_t key)
{
	struct rm_args  rma;
	cm_num_t dev;
	cm_num_t func;

	SET_RM_ARGS(&rma, CM_DEVNUM, key, &dev, CM_NUM_SIZE, 0);
	if (_rm_getval(&rma, UIO_SYSSPACE) != 0){
		return ( (cm_num_t)(-1) );
	}

	SET_RM_ARGS(&rma, CM_FUNCNUM, key, &func, CM_NUM_SIZE, 0);
	if (_rm_getval(&rma, UIO_SYSSPACE) != 0){
		return ( (cm_num_t)(-1) );
	}

	return ( (dev <<5) | func );

}

/*
 * STATIC cm_num_t
 * _cm_get_brdslot(rm_key_t key)
 *
 * Calling/Exit State:
 *      None.
 */
STATIC cm_num_t
_cm_get_brdslot(rm_key_t key)
{
	struct rm_args  rma;
	cm_num_t slot;

	SET_RM_ARGS(&rma, CM_SLOT, key, &slot, CM_NUM_SIZE, 0);
	if (_rm_getval(&rma, UIO_SYSSPACE) == 0){
		return slot;
	}

	return ( (cm_num_t)(-1) );
}

/*
 * size_t
 * cm_devconfig_size(rm_key_t key)
 *	Get the size of device configuration space.
 *
 * Calling/Exit State:
 *	Returns the size of the device configuration space.
 */
size_t
cm_devconfig_size(rm_key_t key)
{
	cm_num_t devconfig;
	ulong_t bustype;

	switch((bustype = (ulong_t)_cm_get_brdbustype(key))) {
	case CM_BUS_EISA:
	case CM_BUS_MCA:
	case CM_BUS_PCI:
	case CM_BUS_UNK:
		break;
	default:
		bustype = CM_BUS_UNK;
		break;
	}

	if (_cm_get_ca_devconfig(key, &devconfig) == -1)
		return 0;

	return ca_devconfig_size(bustype, devconfig);
}

/*
 * int
 * cm_read_devconfig(rm_key_t key, size_t offset, void *buf, size_t nbytes)
 *	Read device configuration space.
 *
 * Calling/Exit State:
 *	None.
 */
int
cm_read_devconfig(rm_key_t key, size_t offset, void *buf, size_t nbytes)
{
	cm_num_t devconfig;
	ulong_t bustype;
	int ret;


	switch((bustype = (ulong_t)_cm_get_brdbustype(key))) {
	case CM_BUS_EISA:
	case CM_BUS_MCA:
	case CM_BUS_PCI:
	case CM_BUS_UNK:
		break;
	default:
		bustype = CM_BUS_UNK;
		break;
	}

	if (_cm_get_ca_devconfig(key, &devconfig) == -1)
		return -1;

	ret = ca_read_devconfig(bustype, devconfig, buf, offset, nbytes);
	return ret;
}

/*
 * int
 * cm_write_devconfig(rm_key_t key, size_t offset, void *buf, size_t nbytes)
 *	Write device configuration space.
 *
 * Calling/Exit State:
 *	None.
 */
int
cm_write_devconfig(rm_key_t key, size_t offset, void *buf, size_t nbytes)
{
	cm_num_t devconfig;
	ulong_t bustype;
	int ret;

	switch((bustype = (ulong_t)_cm_get_brdbustype(key))) {
	case CM_BUS_EISA:
	case CM_BUS_MCA:
	case CM_BUS_PCI:
	case CM_BUS_UNK:
		break;
	default:
		bustype = CM_BUS_UNK;
		break;
	}

	if (_cm_get_ca_devconfig(key, &devconfig) == -1)
		return -1;

	ret = ca_write_devconfig(bustype, devconfig, buf, offset, nbytes);
	return ret;
}


/*
 * STATIC void
 * _cm_itoh(ulong_t n, char s[], int pad)
 *	Internal routine to convert unsigned integer to HEX ASCII.
 *  pad with 0's to "pad" digits.  pad=0 for NO padding.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
_cm_itoh(ulong_t n, char s[], int pad)
{
	char	buf[CM_MAXHEXDIGITS];
	char	*bp;
	int	i;

	bp = buf;
	i = 0;

	do {
		*bp++ = "0123456789ABCDEF"[n % CM_HEXBASE];
		i++;

	} while (((n /= CM_HEXBASE) > 0) && i < CM_MAXHEXDIGITS);

	/* pad to "pad" digits with 0's */

	pad -= bp - buf;

	bp--;

	*s++ = '0';	/* Add standard HEX prefix '0x' */
	*s++ = 'x';

	while ( pad-- > 0 )
		*s++ = '0';

	while (bp >= buf)
		*s++ = *bp--;

	*s = '\0';
}


/*
 * int
 * cm_AT_putconf(rm_key_t rm_key, int vector, int itype,
 *		ulong sioa, ulong eioa,
 *		ulong scma, ulong ecma, int dmac,
 *		uint setmask, int claim)
 *	Populate the internal Resource Manager Database with
 * 	the PARAMETER=values that were interpreted from
 * 	PCU data by the driver.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
cm_AT_putconf(rm_key_t rm_key, int vector, int itype,
		ulong sioa, ulong eioa,
		ulong scma, ulong ecma, int dmac,
		uint setmask, int claim)
{
	cm_args_t		cma;
	cm_num_t		num;
	struct	cm_addr_rng	rng;
	int			rv;

	cma.cm_key = rm_key;
	cma.cm_n   = 0;

	cm_begin_trans(rm_key, RM_RDWR);
	if (setmask & CM_SET_IRQ)	{
		/* Delete and Set IRQ.  */
		cma.cm_param = CM_IRQ;
		cma.cm_vallen = sizeof(cm_num_t);
		(void) cm_delval(&cma);
		num = vector;
		cma.cm_val = &num;
		if ((rv = cm_addval(&cma)) != 0) {
			/*
			 *+ Warning message
			 */
			cmn_err(CE_WARN, "cm_AT_putconf() failed cm_addval()\n");
			cm_end_trans(rm_key);
			return(rv);
		}
	}

	if (setmask & CM_SET_ITYPE)	{
		/* Delete and Set ITYPE.  */
		cma.cm_param = CM_ITYPE;
		cma.cm_vallen = sizeof(cm_num_t);
		(void) cm_delval(&cma);
		num = itype;
		cma.cm_val = &num;
		if ((rv = cm_addval(&cma)) != 0) {
			/*
			 *+ Warning message
			 */
			cmn_err(CE_WARN, "cm_AT_putconf() failed cm_addval()\n");
			cm_end_trans(rm_key);
			return(rv);
		}
	}

	if (setmask & CM_SET_IOADDR)	{
		/* Delete and Set IO address range. */
		cma.cm_param = CM_IOADDR;
		cma.cm_vallen = sizeof(struct cm_addr_rng);
		(void) cm_delval(&cma);
		rng.startaddr = sioa;
		rng.endaddr = eioa;
		cma.cm_val = &rng;
		if ((rv = cm_addval(&cma)) != 0) {
			/*
			 *+ Warning message
			 */
			cmn_err(CE_WARN, "cm_AT_putconf() failed cm_addval()\n");
			cm_end_trans(rm_key);
			return(rv);
		}
	}

	if (setmask & CM_SET_MEMADDR)	{
		/* Delete and Set MEM address range.  */
		cma.cm_param = CM_MEMADDR;
		cma.cm_vallen = sizeof(struct cm_addr_rng);
		(void) cm_delval(&cma);
		rng.startaddr = scma;
		rng.endaddr = ecma;
		cma.cm_val = &rng;
		if ((rv = cm_addval(&cma)) != 0) {
			/*
			 *+ Warning message
			 */
			cmn_err(CE_WARN, "cm_AT_putconf() failed cm_addval()\n");
			cm_end_trans(rm_key);
			return(rv);
		}
	}

	if (setmask & CM_SET_DMAC)	{
		/* Delete and Set DMAC.  */
		cma.cm_param = CM_DMAC;
		cma.cm_vallen = sizeof(cm_num_t);
		(void) cm_delval(&cma);
		num = dmac;
		cma.cm_val = &num;
		if ((rv = cm_addval(&cma)) != 0) {
			/*
			 *+ Warning message
			 */
			cmn_err(CE_WARN, "cm_AT_putconf() failed cm_addval()\n");
			cm_end_trans(rm_key);
			return(rv);
		}
	}

	if (claim != 0) {
		cma.cm_param = CM_CLAIM;
		cma.cm_vallen = sizeof(cm_num_t);
		cma.cm_val = &num;
		num = setmask;
		(void)cm_delval(&cma);
		/* If bitmask is 0, no need to have the CM_CLAIM param */
		if (setmask != 0  &&  (rv = cm_addval(&cma)) != 0) {
			cmn_err(CE_WARN, "cm_AT_putconf() failed CM_CLAIM\n");
			cm_end_trans(rm_key);
			return rv;
		}
	}

	cm_end_trans(rm_key);
	return (0);
}
/*
** _cm_check_param()
**
** Calling/Exit State: None
**
** Description:
**	Returns B_TRUE if param is in list, B_FALSE otherwise.
*/

STATIC boolean_t
_cm_check_param( const char *plist[], const char *param )
{
	while ( *plist != NULL )
		if ( strcmp( *plist++, param ) == 0 )
			return B_TRUE;

	return B_FALSE;
}

/*
** _cm_ro_param()
**
** Calling/Exit State: None
**
** Description:
**	Returns B_TRUE if param is read-only, B_FALSE otherwise.
*/

boolean_t
_cm_ro_param( const char *param )
{
	static const char *ro_params[] = {
					    CM_BINDCPU,
					    CM_BOOTHBA,
					    CM_BRDBUSTYPE,
					    CM_BRDID,
					    CM_BUSNUM,
					    CM_CA_DEVCONFIG,
					    CM_CGID,
					    CM_DEVNUM,
					    CM_FUNCNUM,
					    CM_PCI_VERSION,
					    CM_SBRDID,
					    CM_SLOT,
					    CM_TYPE,
					    NULL
				   	 };

	return _cm_check_param( ro_params, param );
}

/*
 * NOTE: Some functions below assume hidden_params and hidden_params_5
 * are subsets of ro_params.
 */
static const char *hidden_params[] = {
	CM_BINDCPU,
	CM_BOOTHBA,
	/* CM_CA_DEVCONFIG not officially supported, but not enforced */
	CM_CGID,
	NULL
};

static const char *hidden_params_5[] = {
	CM_BUSNUM,
	CM_DEVNUM,
	CM_FUNCNUM,
	CM_SBRDID,
	NULL
};

static const char *hidden_params_8[] = {
	NULL
};

/*
** _Compat_cm_getval_5()
**
** Descriptions:
**	Compatibility support for ddi 5 drivers; pre-transaction model
**	and restricted to ddi 5 vintage parameters.
*/
int
_Compat_cm_getval_5( cm_args_t *cma )
{
	if ( cma == NULL || cma->cm_param == NULL ||
		_cm_check_param( hidden_params, cma->cm_param ) ||
		_cm_check_param( hidden_params_5, cma->cm_param ) )
	{
		return EINVAL;
	}

	return _Compat_cm_getval( cma );
}

/*
** _Compat_cm_getval_7_1()
**
** Descriptions:
**	Compatibility support for ddi 7.1 drivers; pre-transaction model
**	and restricted to ddi 7.1 vintage parameters.
*/
int
_Compat_cm_getval_7_1( cm_args_t *cma )
{
	if ( cma == NULL || cma->cm_param == NULL ||
		_cm_check_param( hidden_params, cma->cm_param ) )
	{
		return EINVAL;
	}

	return _Compat_cm_getval( cma );
}

/*
** _Compat_cm_getval_8()
**
** Descriptions:
**	Compatibility support for ddi 8 drivers;
**	restricted to ddi 8 vintage parameters.
*/
int
_Compat_cm_getval_8( cm_args_t *cma )
{
	if ( cma == NULL || cma->cm_param == NULL || cma->cm_key == RM_KEY ||
		_cm_check_param( hidden_params, cma->cm_param ) ||
		_cm_check_param( hidden_params_8, cma->cm_param ) )
	{
		return EINVAL;
	}

	return cm_getval( cma );
}

/*
** _Compat_cm_addval_8()
**
** Descriptions:
**	Compatibility support for ddi 8 drivers;
**	restricted to ddi 8 vintage parameters.
*/
int
_Compat_cm_addval_8( cm_args_t *cma )
{
	if ( cma == NULL || cma->cm_param == NULL || cma->cm_key == RM_KEY ||
		_cm_check_param( hidden_params_8, cma->cm_param ) )
	{
		return EINVAL;
	}

	return cm_addval( cma );
}

/*
** _Compat_cm_delval_8()
**
** Descriptions:
**	Compatibility support for ddi 8 drivers;
**	restricted to ddi 8 vintage parameters.
*/
int
_Compat_cm_delval_8( cm_args_t *cma )
{
	if ( cma == NULL || cma->cm_param == NULL || cma->cm_key == RM_KEY ||
		_cm_check_param( hidden_params_8, cma->cm_param ) )
	{
		return EINVAL;
	}

	return cm_delval( cma );
}

/*
** int cm_bustypes( uint_t *bustypes )
**
** Calling/Exit State: None
**
** Description:
**	Returns a bit mask of bustypes available on the system.
*/

uint_t
cm_bustypes( void )
{
	return _cm_bustypes;
}

/*
** int _cm_rw_bytes( rm_key_t key, size_t offset, void *buf, int rwflg )
**
** Calling/Exit State: None
**
** Description:
**	Generic interface supporting cm_read/write_devconfig8/16/32 interfaces
*/

STATIC int
_cm_rw_bytes( rm_key_t key, size_t offset, void *buf, int rwflg )
{
	struct rm_args	rma;
	cm_num_t	devconfig;
	cm_num_t	bustype;
	int		ret;

	if ( buf == NULL )
		return EINVAL;

	rma.rm_key = key;
	(void)strcpy( rma.rm_param, CM_BRDBUSTYPE );
	rma.rm_val = &bustype;
	rma.rm_vallen = sizeof( bustype );
	rma.rm_n = 0;

	if ( _rm_getval( &rma, UIO_SYSSPACE ) != 0 ){ /* RISKY */
		return EINVAL;
	}

	(void)strcpy( rma.rm_param, CM_CA_DEVCONFIG );
	rma.rm_val = &devconfig;
	rma.rm_vallen = sizeof( devconfig );
	if ( _rm_getval( &rma, UIO_SYSSPACE ) != 0 ){ /* RISKY */
		return EINVAL;
	}


	ret = (*_ca_rw[ rwflg ])( bustype, devconfig, buf, offset );
	return ret;
}

/*
 * int
 * cm_read_devconfig8( rm_key_t key, size_t offset, uchar_t *buf )
 *	Read device configuration space.
 *
 * Calling/Exit State:
 *	None.
 */

int
cm_read_devconfig8( rm_key_t key, size_t offset, uchar_t *buf )
{
	return _cm_rw_bytes( key, offset, buf, _CM_READ_8 );
}

/*
 * int
 * cm_read_devconfig16( rm_key_t key, size_t offset, ushort_t *buf )
 *	Read device configuration space.
 *
 * Calling/Exit State:
 *	None.
 */

int
cm_read_devconfig16( rm_key_t key, size_t offset, ushort_t *buf )
{
	return _cm_rw_bytes( key, offset, buf, _CM_READ_16 );
}

/*
 * int
 * cm_read_devconfig32( rm_key_t key, size_t offset, uint_t *buf )
 *	Read device configuration space.
 *
 * Calling/Exit State:
 *	None.
 */

int
cm_read_devconfig32( rm_key_t key, size_t offset, uint_t *buf )
{
	return _cm_rw_bytes( key, offset, buf, _CM_READ_32 );
}

/*
 * int
 * cm_write_devconfig8( rm_key_t key, size_t offset, uchar_t *buf )
 *	Write device configuration space.
 *
 * Calling/Exit State:
 *	None.
 */

int
cm_write_devconfig8( rm_key_t key, size_t offset, uchar_t *buf )
{
	return _cm_rw_bytes( key, offset, buf, _CM_WRITE_8 );
}

/*
 * int
 * cm_write_devconfig16( rm_key_t key, size_t offset, ushort_t *buf )
 *	Write device configuration space.
 *
 * Calling/Exit State:
 *	None.
 */

int
cm_write_devconfig16( rm_key_t key, size_t offset, ushort_t *buf )
{
	return _cm_rw_bytes( key, offset, buf, _CM_WRITE_16 );
}

/*
 * int
 * cm_write_devconfig32( rm_key_t key, size_t offset, uint_t *buf )
 *	Write device configuration space.
 *
 * Calling/Exit State:
 *	None.
 */

int
cm_write_devconfig32( rm_key_t key, size_t offset, uint_t *buf )
{
	return _cm_rw_bytes( key, offset, buf, _CM_WRITE_32 );
}

#ifdef CM_TEST

_cm_null_test( rm_key_t key )
{
	uint	bustypes = 0;

	bustypes = cm_bustypes();

	printf( "TEST: cm_bustypes(): bustypes = 0x%x\n", bustypes );

	/* NULL read tests */

	printf( "TEST: cm_read_devconfig8( key, 0, NULL )" );

	if ( cm_read_devconfig8( key, 0, NULL ) == EINVAL )
		printf( " -- PASSED\n" );

	printf( "TEST: cm_read_devconfig16( key, 0, NULL )" );

	if ( cm_read_devconfig16( key, 0, NULL ) == EINVAL )
		printf( " -- PASSED\n" );

	printf( "TEST: cm_read_devconfig32( key, 0, NULL )" );

	if ( cm_read_devconfig32( key, 0, NULL ) == EINVAL )
		printf( " -- PASSED\n" );

	/* NULL write tests */

	printf( "TEST: cm_write_devconfig8( key, 0, NULL )" );

	if ( cm_write_devconfig8( key, 0, NULL ) == EINVAL )
		printf( " -- PASSED\n" );

	printf( "TEST: cm_write_devconfig16( key, 0, NULL )" );

	if ( cm_write_devconfig16( key, 0, NULL ) == EINVAL )
		printf( " -- PASSED\n" );

	printf( "TEST: cm_write_devconfig32( key, 0, NULL )" );

	if ( cm_write_devconfig32( key, 0, NULL ) == EINVAL )
		printf( " -- PASSED\n" );
}

void
_cm_offset_test( rm_key_t key, size_t offset )
{
	uint_t	buf;

	/* offset overflow tests */

	printf( "TEST: cm_read_devconfig8( key, offset, &buf )" );

	if ( cm_read_devconfig8( key, offset, (uchar_t *)&buf ) == EINVAL )
		printf( " -- PASSED\n" );

	printf( "TEST: cm_read_devconfig16( key, offset, &buf )" );

	if ( cm_read_devconfig16( key, offset, (ushort_t *)&buf ) == EINVAL )
		printf( " -- PASSED\n" );

	printf( "TEST: cm_read_devconfig32( key, offset, &buf )" );

	if ( cm_read_devconfig32( key, offset, &buf ) == EINVAL )
		printf( " -- PASSED\n" );

	printf( "TEST: cm_write_devconfig8( key, offset, &buf )" );

	if ( cm_write_devconfig8( key, offset, (uchar_t *)&buf ) == EINVAL )
		printf( " -- PASSED\n" );

	printf( "TEST: cm_write_devconfig16( key, offset, &buf )" );

	if ( cm_write_devconfig16( key, offset, (ushort_t *)&buf ) == EINVAL )
		printf( " -- PASSED\n" );

	printf( "TEST: cm_write_devconfig32( key, offset, &buf )" );

	if ( cm_write_devconfig32( key, offset, &buf ) == EINVAL )
		printf( " -- PASSED\n" );
}

void
_cm_rw_test( rm_key_t key, size_t offset )
{
	uchar_t		buf1 = 13;
	ushort_t	buf2 = 13;
	uint_t		buf4 = 13;
	uint_t		buf = 13;
	int		ret = 1313;
	int		ret2 = 1313;

	printf( "key = %d -- offset = %d\n", key, offset );

	printf( "TEST: cm_read_devconfig8( key, offset, &buf1 )" );
	ret = cm_read_devconfig8( key, offset, &buf1 );
	printf( " -- ret = %d buf = %d\n", ret, (int)buf1 );

	if ( ret == 0  ||  ret == ENOENT )
	{
		ret2 = cm_read_devconfig( key, offset, &buf, 1 );
		printf( "SANITY: cm_read_devconfig(): ret = %d buf = %d\n",
				ret2, buf );

		ret = ret2 = 1313;
		buf = 13;

		printf( "TEST: cm_write_devconfig8( key, offset, &buf1 )" );
		ret = cm_write_devconfig8( key, offset, &buf1 );
		printf( " -- ret = %d buf = %d\n", ret, (int)buf1 );
	}

	ret = 1313;

	printf( "TEST: cm_read_devconfig16( key, offset, &buf2 )" );
	ret = cm_read_devconfig16( key, offset, &buf2 );
	printf( " -- ret = %d buf = %d\n", ret, (int)buf2 );

	if ( ret == 0  ||  ret == ENOENT )
	{
		ret2 = cm_read_devconfig( key, offset, &buf, 2 );
		printf( "SANITY: cm_read_devconfig(): ret = %d buf = %d\n",
				ret2, buf );

		ret = ret2 = 1313;
		buf = 13;

		printf( "TEST: cm_write_devconfig16( key, offset, &buf2 )" );
		ret = cm_write_devconfig16( key, offset, &buf2 );
		printf( " -- ret = %d buf = %d\n", ret, (int)buf2 );
	}

	ret = 1313;

	printf( "TEST: cm_read_devconfig32( key, offset, &buf4 )" );
	ret = cm_read_devconfig32( key, offset, &buf4 );
	printf( " -- ret = %d buf = %d\n", ret, (int)buf4 );

	if ( ret == 0  ||  ret == ENOENT )
	{
		ret2 = cm_read_devconfig( key, offset, &buf, 4 );
		printf( "SANITY: cm_read_devconfig(): ret = %d buf = %d\n",
				ret2, buf );

		ret = ret2 = 1313;
		buf = 13;

		printf( "TEST: cm_write_devconfig32( key, offset, &buf4 )" );
		ret = cm_write_devconfig32( key, offset, &buf4 );
		printf( " -- ret = %d buf = %d\n", ret, (int)buf4 );
	}
}
void
cm_trans_test1()
{
	rm_key_t	k;
	cm_args_t	cma;

	int num = 9;

	printf("TEST: create key, add IPL, save, then delete\n");
	k = cm_newkey(0, FALSE);
	cma.cm_key = k;
	cma.cm_param = CM_IPL;
	cma.cm_vallen = sizeof(cm_num_t);
	cma.cm_val = &num;

	cm_addval(&cma);
	cm_end_trans(cma.cm_key); 
	cm_begin_trans(cma.cm_key, RM_RDWR);
	cm_delval(&cma);
	cm_abort_trans(&cma);
	cm_end_trans;
	
}
#endif /* CM_TEST */

int
cm_add_entry( struct config_info *cip, rm_key_t key )
{
	rm_args_t		rma;
	cm_num_t		slot = -1;
	cm_num_t		ba;			/* bus/device access */
	cm_num_t		bustype;		/* type of bus */
	cm_num_t		devnum;
	cm_num_t		funcnum = -1;
	cm_num_t		busnum;
	cm_num_t		cgnum;
	char			bid[CM_MAXHEXDIGITS];
	char			*brdid = bid;
	int			ret;

	char			sbrdid[CM_MAXHEXDIGITS];

	bustype = cip->ci_busid;

	switch (bustype) {
	case CM_BUS_EISA:
		ba = (cm_num_t)cip->ci_eisaba;
		slot = EISA_SLOT(ba);
		funcnum = cip->ci_eisa_funcnumber;
		brdid = eisa_uncompress((char *)&cip->ci_eisabrdid);
		break;

	case CM_BUS_MCA:
		ba = (cm_num_t)cip->ci_mcaba;
		slot = MCA_SLOT(ba);
		_cm_itoh(cip->ci_mcabrdid, brdid, 4);
		break;

	case CM_BUS_PCI:
		ba = (cm_num_t)cip->ci_pciba;
		slot = (cm_num_t) cip->ci_pcislot;
		funcnum = cip->ci_pci_devfuncnumber & 0x7;
		_cm_itoh(cip->ci_pcibrdid, brdid, 0);
		break;

	default:
		return -1;
	}

	/*
	 * Get a new key and register all the system resources.
	 */

	rma.rm_key = key;

	/* Register bus type */

	(void)strcpy( rma.rm_param, CM_BRDBUSTYPE );
	rma.rm_vallen = sizeof(bustype);
	rma.rm_val = &bustype;

	if ((ret = rm_addval(&rma, UIO_SYSSPACE)) != 0) 
		return ret;

	/* Register device board id */

	(void)strcpy(rma.rm_param, CM_BRDID);
	rma.rm_vallen = strlen( brdid ) + 1;
	rma.rm_val = brdid;

	if ((ret = rm_addval(&rma, UIO_SYSSPACE)) != 0) {
		return ret;
	}

	/*
	 * Register device specific information necessary to read/write
	 * the configuration space.
	 */

	(void)strcpy(rma.rm_param, CM_CA_DEVCONFIG);
	rma.rm_vallen = sizeof(ba);
	rma.rm_val = &ba;

	if ((ret = rm_addval(&rma, UIO_SYSSPACE)) != 0) {
		return ret;
	}

	/* Register slot information */

	if (slot != -1) {
		(void)strcpy(rma.rm_param, CM_SLOT);
		rma.rm_vallen = sizeof(slot);
		rma.rm_val = &slot;

		if ((rm_addval(&rma, UIO_SYSSPACE) != 0)) {
			return ret;
		}
	}

	/* Register function number */

	if ( funcnum != -1 )
	{
		(void)strcpy(rma.rm_param, CM_FUNCNUM);
		rma.rm_vallen = sizeof(funcnum);
		rma.rm_val = &funcnum;

		if ((ret = rm_addval(&rma, UIO_SYSSPACE)) != 0) {
			return ret;
		}
	}

	/* Since we can't interpret NVRAM info on MCA systems--we're done */

	if (cip->ci_busid == CM_BUS_MCA)
		return 0;

	/* Add PCI bus specific params */

	if (cip->ci_busid == CM_BUS_PCI)
	{
		/* C guarantees NO sign extend */

		devnum = (cip->ci_pci_devfuncnumber >> 3);

		(void)strcpy(rma.rm_param, CM_DEVNUM);
		rma.rm_vallen = sizeof(devnum);
		rma.rm_val = &devnum;

		if ((ret = rm_addval(&rma, UIO_SYSSPACE)) != 0) {
			return ret;
		}

		busnum = cip->ci_pci_busnumber;

		(void)strcpy(rma.rm_param, CM_BUSNUM);
		rma.rm_vallen = sizeof(busnum);
		rma.rm_val = &busnum;

		if ((ret = rm_addval(&rma, UIO_SYSSPACE)) != 0) {
			return ret;
		}

		cgnum = cip->ci_pci_cgnum;

		(void)strcpy(rma.rm_param, CM_CGID);
		rma.rm_vallen = sizeof(cgnum);
		rma.rm_val = &cgnum;

		if ((ret = rm_addval(&rma, UIO_SYSSPACE)) != 0) {
			return ret;
		}

		if (cip->ci_pcisbrdid != 0) { /* ul95-33214 */
			_cm_itoh(cip->ci_pcisbrdid, sbrdid, 0);
			(void)strcpy(rma.rm_param, CM_SBRDID);
			rma.rm_vallen = sizeof(sbrdid);
			rma.rm_val = &sbrdid;

			if ((ret = rm_addval(&rma, UIO_SYSSPACE)) != 0) {
				return ret;
			}
		}

		if (cip->ci_pciclassid != 0) {
			_cm_itoh(cip->ci_pciclassid, sbrdid, 4);
			(void)strcpy(rma.rm_param, CM_SCLASSID);
			rma.rm_vallen = sizeof(sbrdid);
			rma.rm_val = &sbrdid;

			if ((ret = rm_addval(&rma, UIO_SYSSPACE)) != 0) {
				return ret;
			}
		}
	}

	return cm_add_vals( rma.rm_key, cip);
}

STATIC int
cm_add_vals( rm_key_t key, struct config_info *cip)
{
	struct config_irq_info	*cirqip;
	struct cm_addr_rng	ioportrng;
	struct cm_addr_rng	memrng;
	rm_args_t		rma;
	cm_num_t		itype_val;
	cm_num_t		itype[MAX_IRQS];
	cm_num_t		claim = 0;
	cm_num_t		irq;
	cm_num_t		dma;
	int			i;

	rma.rm_key = key;
	rma.rm_n = 0;


	/*
	 * Register IRQ.
	 */ 

	rma.rm_vallen = sizeof(irq);
	(void)strcpy(rma.rm_param, CM_IRQ);
	rma.rm_val = &irq;

	for (i = 0; i < (int)cip->ci_numirqs; i++) {
		irq = (cm_num_t)cip->ci_irqline[i];

		if (rm_addval(&rma, UIO_SYSSPACE) != 0) 
			cmn_err(CE_NOTE, "Could not add %s param", rma.rm_param);
	}

	/*
	 * Register ITYPE (interrupt type -- edge/level shared/!shared.
	 */

	for (i = 0; i < (int)cip->ci_numirqs; i++) {
		cirqip = (struct config_irq_info *)&cip->ci_irqattrib[i];

		if (cirqip->cirqi_trigger) {
			/*
			 * This controller uses a level-sensitive
			 * interrupt vector, which can be shared with
			 * any controller for any driver.
			 */
			itype[i] = 4;
		} else if (!cirqip->cirqi_trigger && cirqip->cirqi_type) {
			/*
			 * This controller uses an interrupt vector which
			 * can be shared with any controller for any driver.
			 */
			itype[i] = 3;
		} else if (!cirqip->cirqi_trigger && !cirqip->cirqi_type) {
			/*
			 * This controller uses an interrupt vector which
			 * is not sharable, even with another controller
			 * for the same driver.
			 */
			itype[i] = 1;
		} else {
			/*
			 * No interrupt type information available.
			 */
			itype[i] = 0;
		}
	}

	rma.rm_vallen = sizeof(itype_val);
	(void)strcpy(rma.rm_param, CM_ITYPE);

	rma.rm_val = &itype_val;

	for (i = 0; i < (int)cip->ci_numirqs; i++) {
		itype_val = itype[i];
		if (itype_val == 0)
			continue;

		if (rm_addval(&rma, UIO_SYSSPACE) != 0) 
			cmn_err(CE_NOTE, "Could not add %s param", rma.rm_param);
	}

	/*
	 * Register DMA channel.
	 */

	rma.rm_vallen = sizeof(dma);
	(void)strcpy(rma.rm_param, CM_DMAC);

	rma.rm_val = &dma;

	for (i = 0; i < (int)cip->ci_numdmas; i++) {
		dma = (cm_num_t)cip->ci_dmachan[i];
		if (rm_addval(&rma, UIO_SYSSPACE) != 0) 
			cmn_err(CE_NOTE, "Could not add %s param", rma.rm_param);
	}

	/*
	 * Register I/O port.
	 */

	rma.rm_vallen = sizeof(ioportrng);
	(void)strcpy(rma.rm_param, CM_IOADDR);

	rma.rm_val = &ioportrng;

	for (i = 0; i < (int)cip->ci_numioports; i++) {
		ioportrng.startaddr = (long) cip->ci_ioport_base[i];
		if (cip->ci_ioport_length[i] == 0)
			ioportrng.endaddr = ioportrng.startaddr;
		else if (cip->ci_ioport_length[i] > 0)
			ioportrng.endaddr = ioportrng.startaddr +
				(long) cip->ci_ioport_length[i] - 1;

		if (rm_addval(&rma, UIO_SYSSPACE) != 0) 
			cmn_err(CE_NOTE, "Could not add %s param", rma.rm_param);
	}

	/*
	 * Register Memory Address Range.
	 */

	rma.rm_vallen = sizeof(memrng);
	(void)strcpy(rma.rm_param, CM_MEMADDR);

	rma.rm_val = &memrng;

	for (i = 0; i < (int)cip->ci_nummemwindows; i++) {
		memrng.startaddr = (long) cip->ci_membase[i];
		memrng.endaddr = memrng.startaddr +
				(long) cip->ci_memlength[i] - 1;

		if (rm_addval(&rma, UIO_SYSSPACE) != 0) 
			cmn_err(CE_NOTE, "Could not add %s param", rma.rm_param);
	}
	return 0;
}
