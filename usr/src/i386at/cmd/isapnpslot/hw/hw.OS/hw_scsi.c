/*
 * File hw_scsi.c
 * Information handler for scsi
 *
 * @(#) hw_scsi.c 65.1 97/06/02 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <sys/sysmacros.h>
#include <stdio.h>
#include <errno.h>
#include <memory.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/scsicmd.h>

#include "hw_scsi.h"
#include "hw_util.h"
#include "scsi_data.h"

/*
 * The following is here since <sys/scsi.h> has broken bit field defs
 */
/* #include <sys/scsi.h> */

#define MAGIC_BUS       0xdeadbabe
#define SCSI_MAXLUN	8
#define SCSI_MAXDEVS	16	/* maximum number of each device type
					   supported under SCSI */

typedef struct exten EXTENSION;
struct exten
{
    int		type;		/* type of extension */
    EXTENSION	*next;		/* pointer to next extension */
    char	data[20];	/* most adapters do byte I/O */
};

typedef struct scsi_dev_cfg
{
    int		index;
    char	*dev_name;		/* device prefix */
    dev_t	devnum;			/* device number */
    u_char	ha_num;			/* host adapter number (0, 1, 2, ...) */
    u_char	id;			/* SCSI priority and address */
    u_char	lun;			/* logical unit number of device */
    u_char	distributed;		/* used for multiprocessing purposes */
    int		(*adapter_entry)();	/* host adapter entry point */
    EXTENSION	*dext;			/* extensions to SCSI spec */
} DEVCFG;

typedef struct scsi_ha_cfg
{
    paddr_t	ha_base;		/* host adapter base address */
    char	*ha_name;		/* host adapter prefix */
    u_char	ha_num;			/* host adapter number (0, 1, 2, ...) */
    u_char	dmach;			/* dma channel */
    u_char	vec;			/* interrupt channel */
    int		(*adapter_entry)();	/* host adapter entry point */
    EXTENSION	*hext;			/* extensions to SCSI spec */
} HACFG; 

typedef struct scsi_ha_vect
{
    char	*name;		/* driver name from mdevice */
    HACFG	*cfgp;		/* pointer to SCSI ha cfg array */
    int		(*init)();	/* xxinit routine */
    int		(*start)();	/* xxstart routine */
    int		(*intr)();	/* xxintr routine */
    int		(*halt)();	/* xxhalt routine */
} HAVECT;

/*
 * SCSI basics
 */

const char	* const callme_scsi[] =
{
    "scsi",
    "scsi_bus",
    "scsi_buss",
    "scsi_devs",
    NULL
};

const char	short_help_scsi[] = "Info on SCSI devices";

static u_char	scsi_bus_cnt = 0;
static HAVECT	*scsi_ha_vect = NULL;
static DEVCFG	**scsi_dev_vect = NULL;

static char	ErrMsg[] = "<ERROR>";

typedef struct
{
    dev_t	dev;
    u_long	mask;
} dev_qual_t;

static int get_ha_vect(void);
static const char *get_card_name(const char *driver);
static int get_dev_vect(void);
static void show_scsi_cfg(FILE *out, int *first, int (*entry)(),
				u_char bus, u_char id, u_char lun);
static void show_id(FILE *out, u_char id, u_char lun);
static const DEVCFG *find_scsi_cfg(int (*entry)(),
				u_char bus, u_char id, u_char lun);
static void show_scsi_devs(FILE *out, int *first, int (*entry)(),
				u_char bus, u_char id, u_char lun);
static dev_t make_scsi_dev(const char *driver, int maj, u_char id);
static void show_dev(FILE *out, int *first, dev_t dev, u_char id, u_char lun);
static void show_scsi_inq(FILE *out, int *first, int (*entry)(),
				u_char ha, u_char bus, u_char id, u_char lun);
static int open_scsi_bus(int (*entry)(), u_char ha, u_char bus, u_char id);

int
have_scsi(void)
{
    get_ha_vect();

    return scsi_bus_cnt;
}

void
report_scsi(FILE *out)
{
    u_char	ha;

    report_when(out, "SCSI");

    if (!have_scsi())
    {
	fprintf(out, "    No SCSI configured\n");
	return;
    }

    load_scsi_data();

    if (get_dev_vect())
	return;

    fprintf(out, "    SCSI buses:\t%hu\n", scsi_bus_cnt);

    /*
     * Run the host adapter drivers
     */

    for (ha = 0; scsi_ha_vect[ha].name; ++ha)
    {
	u_char		bus;
	const char	*name;

	if (!scsi_ha_vect[ha].name || !scsi_ha_vect[ha].cfgp)
	    continue;

	fprintf(out, "\n    Adapter %lu\n", ha);

	fprintf(out, "\tAdapter name:  %s", scsi_ha_vect[ha].name);
	if ((name = get_card_name(scsi_ha_vect[ha].name)) != NULL)
	    fprintf(out, " -  %s", name);
	fprintf(out, "\n");

	/*
	 * Run the busses on this SCSI driver
	 */

	for (bus = 0; scsi_ha_vect[ha].cfgp[bus].ha_base != 0xff; ++bus)
	{
	    u_char	id;
	    int         (*entry)();
	    int		found;

	    fprintf(out, "\tSCSI Bus:      %lu\n",
					scsi_ha_vect[ha].cfgp[bus].ha_num);

	    fprintf(out, "\t    Adapter Driver:  ");
	    if (scsi_ha_vect[ha].cfgp[bus].ha_name)
		fprintf(out, "%s%lu\n",
				    scsi_ha_vect[ha].cfgp[bus].ha_name,
				    scsi_ha_vect[ha].cfgp[bus].ha_num);
	    else
		fprintf(out, "%lu\n", scsi_ha_vect[ha].cfgp[bus].ha_num);

	    /*
	     * Run the IDs on this SCSI bus
	     */

	    entry = scsi_ha_vect[ha].cfgp[bus].adapter_entry;
	    found = 0;
	    for (id = 0; id < SCSI_MAXDEVS; ++id)
	    {
		u_char	lun;
		for (lun = 0; lun < SCSI_MAXLUN; ++lun)
		{
		    int	first = 1;

		    show_scsi_cfg(out, &first, entry, bus, id, lun);
		    show_scsi_devs(out, &first, entry, bus, id, lun);
		    show_scsi_inq(out, &first, entry, ha, bus, id, lun);

		    if (!first)
			found = 1;
		}
	    }

	    if (!found)
		fprintf(out,
		    "\t    No devices configured or found on this bus!\n");

	    fprintf(out, "\n");
	}
    }
}

static const char *
get_card_name(const char *driver)
{
    static const char	drNames[] = "/etc/default/scsihas";
    FILE		*fd;
    static char		buf[256];
    int			dlen;

    if (!(fd = fopen(drNames, "r")))
	return NULL;

    dlen = strlen(driver);
    while (fgets(buf, sizeof(buf), fd))
    {
	char	*tp;
	int	n;

	n = strlen(buf);
	while ((n > 0) && isspace(buf[n-1]))
	    buf[--n] = '\0';

	for (tp = buf; isspace(*tp); ++tp)
	    ;

	if (!*tp || (*tp == '#'))
	    continue;			/* Ignore blank and comment lines */

	if ((strncmp(driver, tp, dlen) != 0) || !isspace(tp[dlen]))
	    continue;			/* Not our driver */

	tp = &tp[dlen];
	while (isspace(*tp))
	    ++tp;

	if (*tp == '"')
	{
	    ++tp;
	    for ( ; isspace(*tp); ++tp)
		;

	    n = strlen(tp);
	    if ((n > 0) && (tp[n-1] == '"'))
		tp[--n] = '\0';

	    while ((n > 0) && isspace(tp[n-1]))
		tp[--n] = '\0';
	}

	if (*tp)
	{
	    fclose(fd);
	    return tp;
	}
    }

    fclose(fd);
    return NULL;
}

static int
get_ha_vect()
{
    u_long	scsi_ha_vect_addr;
    u_char	ha;
    u_long	addr;
    u_char	ha_cnt;

    if (scsi_ha_vect)
	return 0;

    if (get_kaddr("scsi_ha_vect", &scsi_ha_vect_addr))
	return -1;

    /*
     * Count the size of scsi_ha_vect[]
     */

    for (ha_cnt = 0; ; ++ha_cnt)
    {
	HAVECT	scsi_ha;

	addr = scsi_ha_vect_addr + (sizeof(HAVECT) * ha_cnt);
	if (read_kmem(addr, &scsi_ha, sizeof(HAVECT)) != sizeof(HAVECT))
	    return -1;

	if (!scsi_ha.name)
	    break;
    }

    if (!ha_cnt)
	return 0;

    /*
     * Load scsi_ha_vect[]
     */

    if (!(scsi_ha_vect = (HAVECT *)malloc(sizeof(HAVECT) * ha_cnt + 1)))
	return errno = ENOMEM;

    scsi_bus_cnt = 0;
    for (ha = 0; ha <= ha_cnt ; ++ha)
    {
	const char	*name;
	u_long		cfg_addr;
	u_char		cfg_cnt;
	u_char		bus;

	addr = scsi_ha_vect_addr + (sizeof(HAVECT) * ha);
	if (read_kmem(addr, &scsi_ha_vect[ha], sizeof(HAVECT))!=sizeof(HAVECT))
	{
	    free(scsi_ha_vect);
	    scsi_ha_vect = NULL;
	    scsi_bus_cnt = 0;
	    return -1;
	}

	if (!scsi_ha_vect[ha].name || !scsi_ha_vect[ha].cfgp)
	    continue;

	/*
	 * Get the name
	 */

	if (!(name = read_k_string((u_long)scsi_ha_vect[ha].name)))
	    scsi_ha_vect[ha].name = ErrMsg;
	else
	    scsi_ha_vect[ha].name = strdup(name);

	/*
	 * Get the configuration
	 */

	cfg_addr = (u_long)scsi_ha_vect[ha].cfgp;
	for (cfg_cnt = 0; ; ++cfg_cnt)
	{
	    HACFG	cfgp;

	    addr = cfg_addr + (sizeof(HACFG) * cfg_cnt);
	    if (read_kmem(addr, &cfgp, sizeof(HACFG)) != sizeof(HACFG))
	    {
		free(scsi_ha_vect);
		scsi_ha_vect = NULL;
		scsi_bus_cnt = 0;
		return -1;
	    }

	    if (cfgp.ha_base == 0xff)
		break;
	}

	if (!cfg_cnt)
	{
	    scsi_ha_vect[ha].cfgp = NULL;
	    continue;
	}

	if (!(scsi_ha_vect[ha].cfgp =
				(HACFG *)malloc(sizeof(HACFG) * cfg_cnt + 1)))
	{
	    free(scsi_ha_vect);
	    scsi_ha_vect = NULL;
	    scsi_bus_cnt = 0;
	    return errno = ENOMEM;
	}

	for (bus = 0; bus <= cfg_cnt; ++bus)
	{
	    addr = cfg_addr + (sizeof(HACFG) * bus);
	    if (read_kmem(addr, &scsi_ha_vect[ha].cfgp[bus], sizeof(HACFG)) !=
								sizeof(HACFG))
	    {
		free(scsi_ha_vect);
		scsi_ha_vect = NULL;
		scsi_bus_cnt = 0;
		return -1;
	    }

	    if (scsi_ha_vect[ha].cfgp[bus].ha_base == 0xff)
		continue;

	    ++scsi_bus_cnt;

	    if (scsi_ha_vect[ha].cfgp[bus].ha_name)
	    {
		name= read_k_string((u_long)scsi_ha_vect[ha].cfgp[bus].ha_name);
		if (name && *name)
		    scsi_ha_vect[ha].cfgp[bus].ha_name = strdup(name);
		else
		    scsi_ha_vect[ha].cfgp[bus].ha_name = NULL;
	    }
	}
    }

    return 0;
}

static int
get_dev_vect()
{
    u_long	scsi_dev_vect_addr;
    u_char	dev;
    u_long	addr;
    u_char	dev_cnt;

    if (scsi_dev_vect)
	return 0;

    if (get_kaddr("scsi_dev_vect", &scsi_dev_vect_addr))
	return -1;

    /*
     * Count the size of scsi_dev_vect[]
     */

    for (dev_cnt = 0; ; ++dev_cnt)
    {
	u_long	dp;

	addr = scsi_dev_vect_addr + (sizeof(DEVCFG *) * dev_cnt);
	if (read_kmem(addr, &dp, sizeof(u_long)) != sizeof(u_long))
	    return -1;

	if (!dp)
	    break;
    }

    if (!dev_cnt)
	return 0;

    /*
     * Load scsi_dev_vect[]
     */

    if (!(scsi_dev_vect = (DEVCFG **)malloc(sizeof(DEVCFG *) * dev_cnt + 1)))
	return errno = ENOMEM;

    for (dev = 0; dev <= dev_cnt ; ++dev)
    {
	u_char		cfg_cnt;
	u_long		cfg_addr;
	u_char		cfg;
	DEVCFG		*cp;

	addr = scsi_dev_vect_addr + (sizeof(DEVCFG *) * dev);
	if (read_kmem(addr, &cfg_addr, sizeof(DEVCFG *)) != sizeof(DEVCFG *))
	    return -1;

	if (!cfg_addr)
	{
	    scsi_dev_vect[dev] = NULL;
	    break;
	}

	/*
	 * Get the configuration
	 */

	for (cfg_cnt = 0; ; ++cfg_cnt)
	{
	    DEVCFG	dev_cfg;

	    addr = cfg_addr + (sizeof(DEVCFG) * cfg_cnt);
	    if (read_kmem(addr, &dev_cfg, sizeof(DEVCFG)) != sizeof(DEVCFG))
	    {
		free(scsi_dev_vect);
		scsi_dev_vect = NULL;
		return -1;
	    }

	    if (dev_cfg.index == 0xff)
		break;
	}

	if (!(cp = (DEVCFG *)malloc(sizeof(DEVCFG) * cfg_cnt + 1)))
	{
	    free(scsi_dev_vect);
	    scsi_dev_vect = NULL;
	    return errno = ENOMEM;
	}

	scsi_dev_vect[dev] = cp;
	for (cfg = 0; cfg <= cfg_cnt; ++cfg)
	{
	    const char	*name;

	    addr = cfg_addr + (sizeof(DEVCFG) * cfg);
	    if (read_kmem(addr, &cp[cfg], sizeof(DEVCFG)) != sizeof(DEVCFG))
	    {
		free(scsi_dev_vect);
		scsi_dev_vect = NULL;
		return -1;
	    }

	    if (cp[cfg].dev_name)
	    {
		name = read_k_string((u_long)cp[cfg].dev_name);
		if (name && *name)
		{
		    cp[cfg].dev_name = strdup(name);
		}
		else
		    cp[cfg].dev_name = NULL;
	    }

	    if (cp[cfg].dext)
	    {
		EXTENSION		dext;
		static EXTENSION	NullExt =
		{
		    (int)MAGIC_BUS,
		    NULL,
		    { '\xff', }
		};

		if (read_kmem((u_long)cp[cfg].dext, &dext, sizeof(EXTENSION)) !=
							    sizeof(EXTENSION))
		    cp[cfg].dext = &NullExt;
		else if (!(cp[cfg].dext=(EXTENSION *)malloc(sizeof(EXTENSION))))
		   cp[cfg].dext = &NullExt;
		else
		    *cp[cfg].dext = dext;
	    }
	}
    }

    return 0;
}

static void
show_scsi_cfg(FILE *out, int *first, int (*entry)(),
					    u_char bus, u_char id, u_char lun)
{
    const DEVCFG	*cp;

    if (!(cp = find_scsi_cfg(entry, bus, id, lun)))
	return;

    if (*first)
    {
	*first = 0;
	show_id(out, id, lun);
    }

    fprintf(out, "\t\ttarget driver:  %s\n", cp->dev_name);
    fprintf(out, "\t\tDev major:      %u\n", cp->devnum);
}

static void
show_id(FILE *out, u_char id, u_char lun)
{
    fprintf(out, "\t    Target ID:       %hu", id);

    if (lun)
	fprintf(out, ", LUN: %hu", lun);

    fprintf(out, "\n");
}

static const DEVCFG *
find_scsi_cfg(int (*entry)(), u_char bus, u_char id, u_char lun)
{
    u_char	dev;

    if (!scsi_dev_vect)
	return NULL;	/* Could not find SCSI */

    for (dev = 0; scsi_dev_vect[dev] ; ++dev)
    {
	u_char	cfg;
	DEVCFG	*cp = scsi_dev_vect[dev];

	for (cfg = 0; cp[cfg].index != 0xff; ++cfg)
	    if ((cp[cfg].devnum != 0) &&
		(entry == cp[cfg].adapter_entry) &&	/* ha */
		(cp[cfg].dext != NULL) &&
		(bus == cp[cfg].dext->data[0]) &&	/* bus */
		(id == cp[cfg].id) &&			/* id */
		(lun == cp[cfg].lun))			/* lun */
		    return &cp[cfg];
    }

    return NULL;
}

/*
 * Find device names and filesystems for this device.
 */

static void
show_scsi_devs(FILE *out, int *first, int (*entry)(),
					u_char bus, u_char id, u_char lun)
{
    const DEVCFG	*cp;
    int			fd;
    u_char		device;

    if (!verbose ||		/* If we do not care about these details */
	!scsi_dev_vect)		/*	or we could not find any SCSI */
	    return;		/*	get outa here. */

    for (device = 0; scsi_dev_vect[device] ; ++device)
    {
	u_char	cfg;
	DEVCFG	*cp = scsi_dev_vect[device];

	for (cfg = 0; cp[cfg].index != 0xff; ++cfg)
	    if ((cp[cfg].devnum != 0) &&
		(entry == cp[cfg].adapter_entry) &&	/* ha */
		(cp[cfg].dext != NULL) &&
		(bus == cp[cfg].dext->data[0]))		/* bus */
	    {
		dev_t dev = make_scsi_dev(cp[cfg].dev_name, cp[cfg].devnum, id);

		if (dev != NODEV)
		    show_dev(out, first, dev, id, lun);
	    }
    }
}

/*
 * Use our knowledge of the target driver to make a device on
 * the correct bus.  Pick a single minor for that device that
 * will most likely work for us.
 */

static dev_t
make_scsi_dev(const char *driver, int maj, u_char id)
{
    dev_t	dev;

    if (strcmp(driver, "Sdsk") == 0)
    {
	/*
	 * The minor device numbering scheme for SCSI hard disk
	 * drives is the same as the standard minor device
	 * number scheme for non-SCSI disk devices (see hd(HW)).
	 *
	 * We will use the minor of the whole device.
	 * See also:
	 *	scsi_getdev(unit, Sdskcfg, Sdskopen, Sdskname)
	 *	unit = DPHYS(minor(dev))
	 */

	dev = makedev(maj, (u_long)id << 6);
    }
#ifdef NOT_YET	/* ## */
    else if (strcmp(driver, "Stp") == 0)
    {
	/*
	 * The minor device numbering scheme for SCSI tape
	 * drives is described on scsitape(HW).
	 *
	 * We will use the minor of the control device.
	 */

	dev = makedev(maj, #);
    }
    else if (driver, "Srom") == 0)
    {
	/*
	 * The minor device numbering scheme for SCSI CD-ROM
	 * drives is described on cdrom(HW).
	 */

	dev = makedev(maj, #);
    }
    else if (driver, "Sflp") == 0)
    {
	/*
	 * The minor device numbering scheme for SCSI floptical
	 * drives is described on floptical(HW).
	 */

	dev = makedev(maj, #);
    }
    else if (driver, "Sjk") == 0)
    {
	/*
	 * The minor device numbering scheme for SCSI juke is ##
	 */

	dev = makedev(maj, #);
    }
#endif
    else
	dev = NODEV;

    return dev;
}

static void
show_dev(FILE *out, int *first, dev_t dev, u_char id, u_char lun)
{
#ifdef NOT_YET	/* ## */

    if (*first)
    {
	*first = 0;
	show_id(out, id, lun);
    }

    fprintf(out, "\t\ttarget driver:  %s\n", cp->dev_name);

    /* ## search mnttab */
    /* ## search /dev */
#endif
}

static void
show_scsi_inq(FILE *out, int *first, int (*entry)(),
				u_char ha, u_char bus, u_char id, u_char lun)
{
    int			fd;
    struct scsicmd	cmd;
    u_char		*cdb;
    u_char		scsi_data[36];
    u_char		dev_type;
    const char		*dev_type_str;
    u_long		vSize;
    u_long		bSize;

    if (lun)
	return;		/* ## */

    if ((fd = open_scsi_bus(entry, ha, bus, id)) == -1)
	return;

    /*
     * SCSI INQUIRY
     */

    cmd.data_ptr = (char *)scsi_data;
    cmd.data_len = sizeof(scsi_data);
    cmd.is_write = 0;
    cmd.host_sts = 0;
    cmd.target_sts = 0;
    cmd.cdb_len = 6;

    memset(cmd.cdb, 0, sizeof(cmd.cdb));
    cdb = cmd.cdb;
    *cdb++ = 0x12;		/* INQUIRY */
    *cdb++ = (lun & 7) << 5;
    *cdb++ = 0;
    *cdb++ = 0;
    *cdb = sizeof(scsi_data);

    if (ioctl(fd, SCSIUSERCMD, &cmd) ||
	(cmd.host_sts != 0) ||
	(cmd.target_sts != 0) ||
	(scsi_data[0] & 0x60))
    {
	close(fd);
	return;
    }

    dev_type_str = NULL;
    switch (dev_type = (scsi_data[0] & 0x1fU))
    {
	case 0x00:
	    dev_type_str = "Disk";
	    break;

	case 0x01:
	    dev_type_str = "Tape";
	    break;

	case 0x02:
	    dev_type_str = "Printer";
	    break;

	case 0x03:
	    dev_type_str = "Processor";
	    break;

	case 0x04:
	    dev_type_str = "WORM";
	    break;

	case 0x05:
	    dev_type_str = "CD-ROM";
	    break;

	case 0x06:
	    dev_type_str = "Scanner";
	    break;

	case 0x07:
	    dev_type_str = "Optical Memory";
	    break;

	case 0x08:
	    dev_type_str = "Juke";
	    break;

	case 0x09:
	    dev_type_str = "Communications Device";
	    break;
    }

    if (!dev_type_str)
    {
	close(fd);
	return;
    }

    if (*first)
    {
	*first = 0;
	show_id(out, id, lun);
	fprintf(out, "\t\tNOTICE:         No kernel configuration\n");
    }

    fprintf(out, "\t\tDevice type:    %s", dev_type_str);
    if (scsi_data[1] & 0x80U)
	fprintf(out, ", Removable media");
    fprintf(out, "\n");

    if (scsi_data[4] >= 31)
    {
	fprintf(out, "\t\tVendor:         %.8s\n", &scsi_data[8]);
	fprintf(out, "\t\tProduct:        %.16s\n", &scsi_data[16]);
	fprintf(out, "\t\tRevision:       %.4s\n", &scsi_data[32]);
    }

    /*
     * SCSI READ CAPACITY
     */

    if (dev_type != 0x00)	/* Disk */
    {
	close(fd);
	return;
    }

    cmd.data_ptr = (char *)scsi_data;
    cmd.data_len = 8;
    cmd.is_write = 0;
    cmd.host_sts = 0;
    cmd.target_sts = 0;
    cmd.cdb_len = 10;

    memset(cmd.cdb, 0, sizeof(cmd.cdb));
    cdb = cmd.cdb;
    *cdb++ = 0x25;		/* READ CAPACITY */
    *cdb = (lun & 7) << 5;

    if (ioctl(fd, SCSIUSERCMD, &cmd) ||
	(cmd.host_sts != 0) ||
	(cmd.target_sts != 0))
    {
	close(fd);
	return;
    }

    vSize = ((u_long)scsi_data[0] << 24) |
	    ((u_long)scsi_data[1] << 16) |
	    ((u_long)scsi_data[2] << 8) |
	     (u_long)scsi_data[3] + 1;

    bSize = ((u_long)scsi_data[4] << 24) |
	    ((u_long)scsi_data[5] << 16) |
	    ((u_long)scsi_data[6] << 8) |
	     (u_long)scsi_data[7];

    fprintf(out, "\t\tCapacity:       %lu, %lu byte sectors (%.2f Mb)\n",
			    vSize, bSize,
			    ((double)vSize * (double)bSize) / (1024 * 1024));
    close(fd);
}

static int
open_scsi_bus(int (*entry)(), u_char ha, u_char bus, u_char id)
{
    const DEVCFG	*cp;
    int			fd;
    u_char		device;

    if (!scsi_dev_vect)
	return -1;	/* Could not find SCSI */

    /*
     * We need to find at least one target driver for this bus. 
     * Any ID will do.
     */

    for (device = 0; scsi_dev_vect[device] ; ++device)
    {
	u_char	cfg;
	DEVCFG	*cp = scsi_dev_vect[device];

	for (cfg = 0; cp[cfg].index != 0xff; ++cfg)
	    if ((cp[cfg].devnum != 0) &&
		(entry == cp[cfg].adapter_entry) &&	/* ha */
		(cp[cfg].dext != NULL) &&
		(bus == cp[cfg].dext->data[0]))		/* bus */
	    {
		dev_t dev = make_scsi_dev(cp[cfg].dev_name, cp[cfg].devnum, id);

		if (dev == NODEV)
		    continue;

		fd = open_cdev(major(dev), minor(dev), O_RDWR);

		debug_print(
		   "open_scsi_bus(ha=%hu, bus=%hu, id=%hu) dev=%u/%u fd=%d",
				ha, bus, id, major(dev), minor(dev), fd);

		if (fd != -1)
		    return fd;
	    }
    }

    return -1;
}

