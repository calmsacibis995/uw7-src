/*
 * File scsi_data.c
 *
 * @(#) scsi_data.c 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include "hw_util.h"
#include "scsi_data.h"

scsi_asc_t	*scsi_asc = NULL;
scsi_op_t	*scsi_op = NULL;
scsi_vend_t	*scsi_vendors = NULL;

/*
 * This array is matched to:
 *	Field 3 of scsi_asc.cfg
 *	Field 2 of scsi_op.cfg
 * id must be upper case
 */

typedef struct scsi_dev_s
{
    char	id;
    u_long	flag;
    const char	*name;
} scsi_dev_t;

#define ASC_flag_D	0x00000001
#define ASC_flag_T	0x00000002
#define ASC_flag_L	0x00000004
#define ASC_flag_P	0x00000008
#define ASC_flag_W	0x00000010
#define ASC_flag_R	0x00000020
#define ASC_flag_S	0x00000040
#define ASC_flag_O	0x00000080
#define ASC_flag_M	0x00000100
#define ASC_flag_C	0x00000200
#define ASC_flag_A	0x00000400
#define ASC_flag_E	0x00000800
#define ASC_flag_NN	0x80000000

static const scsi_dev_t	scsi_devs[] =
{
    { 'D', ASC_flag_D, "DIRECT ACCESS DEVICE (SBC)" },
    { 'T', ASC_flag_T, "SEQUENTIAL ACCESS DEVICE (SSC)" },
    { 'L', ASC_flag_L, "PRINTER DEVICE (SSC)" },
    { 'P', ASC_flag_P, "PROCESSOR DEVICE (SPC)" },
    { 'W', ASC_flag_W, "WRITE ONCE READ MULTIPLE DEVICE (SBC)" },
    { 'R', ASC_flag_R, "READ ONLY (CD-ROM) DEVICE (MMC)" },
    { 'S', ASC_flag_S, "SCANNER DEVICE (SGC)" },
    { 'O', ASC_flag_O, "OPTICAL MEMORY DEVICE (SBC)" },
    { 'M', ASC_flag_M, "MEDIA CHANGER DEVICE (SMC)" },
    { 'C', ASC_flag_C, "COMMUNICATION DEVICE (SSC)" },
    { 'A', ASC_flag_A, "STORAGE ARRAY DEVICE (SCC)" },
    { 'E', ASC_flag_E, "ENCLOSURE SERVICES DEVICE (SES)" },
};
static const int	scsi_dev_cnt = sizeof(scsi_devs)/sizeof(*scsi_devs);

/*
 * This array is for field 2 of scsi_op.cfg
 * mark must be upper case
 */

typedef struct scsi_key_name_s
{
    char		mark;
    scsi_dev_key_t	key;
    const char		*descr;
} scsi_key_name_t;

static const scsi_key_name_t	scsi_dev_keys[] =
{
    { 'M',	scsi_key_mandatory,	"Mandatory" },
    { 'O',	scsi_key_optional,	"Optional" },
    { 'V',	scsi_key_vendor_spec,	"Vendor specific" },
    { 'R',	scsi_key_reserved,	"Reserved" },
    { 'Z',	scsi_key_obsolete,	"Obsolete" },
};
static const int dev_keys_cnt = sizeof(scsi_dev_keys)/sizeof(*scsi_dev_keys);

const char	asc_devs[] = "DTLPWRSOMCAE";

static int load_scsi_asc(void);
static int load_scsi_op(void);
static int load_scsi_vendor(void);

int
load_scsi_data()
{
    return  load_scsi_asc() ||
	    load_scsi_op() ||
	    load_scsi_vendor();
}

static int
load_scsi_asc()
{
    static const char	cfg_file[] = "scsi_asc.cfg";
    const char		*filename;
    FILE		*fd;
    char		buf[256];
    u_long		line;
    int			err = 0;
    scsi_asc_t		**app;

    if (!(filename = get_lib_file_name(cfg_file)))
	return errno;

    if (!(fd = fopen(filename, "r")))
	return 0;

    app = &scsi_asc;
    line = 0;
    while (fgets(buf, sizeof(buf), fd))
    {
	int		n;
	char		*tp;
	scsi_asc_t	*ap;
	u_long		asc;
	u_long		ascq;
	u_long		flags;
	char		*descr;


	++line;
	n = strlen(buf);
	while ((n > 0) && isspace(buf[n-1]))
	    buf[--n] = '\0';

	for (tp = buf; isspace(*tp); ++tp)
	    ;

	if (!*tp || (*tp == '#'))
	    continue;			/* Ignore blank and comment lines */

	asc = strtoul(tp, &tp, 16);
	if ((asc > UCHAR_MAX) || !tp || (tolower(*tp) != 'h'))
	{
	    debug_print("%s: line %lu: Invalid ASC", filename, line);
	    continue;
	}

	do
	    ++tp;
	while (isspace(*tp));

	if ((toupper(tp[0]) == 'N') &&
	    (toupper(tp[1]) == 'N') &&
	    (tolower(tp[2]) == 'h'))
	{
	    flags = ASC_flag_NN;
	    tp += 3;
	}
	else
	{
	    flags = 0;
	    ascq = strtoul(tp, &tp, 16);
	    if ((ascq > UCHAR_MAX) || !tp || (tolower(*tp) != 'h'))
	    {
		debug_print("%s: line %lu: Invalid ASCQ", filename, line);
		continue;
	    }

	    ++tp;
	}

	if (isspace(tp[0]) && isspace(tp[1]))
	    tp += 2;

	for (n = 0; n < scsi_dev_cnt; ++n)
	{
	    if (!*tp)
		break;

	    if (toupper(*tp) == scsi_devs[n].id)
		flags |= scsi_devs[n].flag;
	    else if (!isspace(*tp))
		debug_print("%s: line %lu: Invalid flag '%c', expecting '%c'",
					filename, line, *tp, scsi_devs[n].id);

	    ++tp;
	}

	if (*tp)
	{
	    if (!isspace(*tp))
		*tp == '\0';

	    while (*tp && !isspace(*tp))
		++tp;
	}

	if (*tp)
	    descr = tp;
	else
	    descr = "<unknown>";

	/*
	 * Find a place to put this new vendor.
	 * We will attach it to the end of the list.
	 */

	if (!(ap = (scsi_asc_t *)malloc(sizeof(scsi_asc_t))))
	{
	    err = ENOMEM;
	    break;
	}

	if (!(ap->descr = strdup(descr)))
	{
	    free(ap);
	    err = ENOMEM;
	    break;
	}

	ap->asc = (u_char)asc;
	ap->ascq = (u_char)ascq;
	ap->flags = flags;
	ap->next = NULL;

	*app = ap;
	app = &(*app)->next;
    }

    fclose(fd);
    debug_print("%s: lines read %lu", filename, line);
    return errno = err;
}

static int
load_scsi_op()
{
    static const char	cfg_file[] = "scsi_op.cfg";
    const char		*filename;
    FILE		*fd;
    char		buf[256];
    u_long		line;
    int			err = 0;
    scsi_op_t		**opp;
    scsi_dev_key_t	*flags;


    if (!(filename = get_lib_file_name(cfg_file)))
	return errno;

    if (!(fd = fopen(filename, "r")))
	return 0;

    opp = &scsi_op;
    line = 0;
    flags = NULL;
    while (fgets(buf, sizeof(buf), fd))
    {
	int		n;
	char		*tp;
	scsi_op_t	*op;
	u_long		optn;
	char		*descr;


	++line;
	n = strlen(buf);
	while ((n > 0) && isspace(buf[n-1]))
	    buf[--n] = '\0';

	for (tp = buf; isspace(*tp); ++tp)
	    ;

	if (!*tp || (*tp == '#'))
	    continue;			/* Ignore blank and comment lines */

	optn = strtoul(tp, &tp, 16);
	if ((optn > UCHAR_MAX) || !tp || !isspace(*tp))
	{
	    debug_print("%s: line %lu: Invalid SCSI OP", filename, line);
	    continue;
	}

	if (!isspace(tp[0]) || !isspace(tp[0]))
	{
	    debug_print("%s: line %lu: Format error", filename, line);
	    continue;
	}

	tp += 2;

	if (!(flags = malloc(sizeof(scsi_dev_key_t)*scsi_dev_cnt)))
	{
	    err = ENOMEM;
	    break;
	}

	for (n = 0; n < scsi_dev_cnt; ++n)
	{
	    if (!*tp)
		break;

	    if (isspace(*tp))
		flags[n] = scsi_key_none;
	    else
	    {
		int	j;

		for (j = 0; j < dev_keys_cnt; ++j)
		    if (toupper(*tp) == scsi_dev_keys[j].mark)
			break;

		if (j < scsi_dev_cnt)
		    flags[n] = scsi_dev_keys[j].mark;
		else
		{
		    flags[n] = scsi_key_none;
		    debug_print("%s: line %lu: Invalid dev flag",
							    filename, line);
		}
	    }

	    ++tp;
	}

	if (*tp)
	{
	    if (!isspace(*tp))
		*tp == '\0';

	    while (*tp && !isspace(*tp))
		++tp;
	}

	if (*tp)
	    descr = tp;
	else
	    descr = "<unknown>";

	/*
	 * Find a place to put this new vendor.
	 * We will attach it to the end of the list.
	 */

	if (!(op = (scsi_op_t *)malloc(sizeof(scsi_op_t))))
	{
	    err = ENOMEM;
	    break;
	}

	if (!(op->descr = strdup(descr)))
	{
	    free(op);
	    err = ENOMEM;
	    break;
	}

	op->op = (u_char)optn;
	op->flags = flags;
	flags = NULL;
	op->next = NULL;

	*opp = op;
	opp = &(*opp)->next;
    }

    if (flags)
	free(flags);
    fclose(fd);
    debug_print("%s: lines read %lu", filename, line);
    return errno = err;
}

static int
load_scsi_vendor()
{
    static const char	cfg_file[] = "scsi_vendor.cfg";
    const char		*filename;
    FILE		*fd;
    char		buf[256];
    u_long		line;
    int			err = 0;
    scsi_vend_t		**vpp;


    if (!(filename = get_lib_file_name(cfg_file)))
	return errno;

    if (!(fd = fopen(filename, "r")))
	return 0;

    vpp = &scsi_vendors;
    line = 0;
    while (fgets(buf, sizeof(buf), fd))
    {
	int		n;
	char		*tp;
	scsi_vend_t	*vp;
	char		*id;
	char		*name;


	++line;
	n = strlen(buf);
	while ((n > 0) && isspace(buf[n-1]))
	    buf[--n] = '\0';

	for (tp = buf; isspace(*tp); ++tp)
	    ;

	if (!*tp || (*tp == '#'))
	    continue;			/* Ignore blank and comment lines */

	id = tp;

	tp += 8;
	if (!isspace(*tp))
	{
	    debug_print("%s: line %lu: Format error", filename, line);
	    continue;
	}

	*tp = '\0';
	n = strlen(id);
	while ((n > 0) && isspace(id[n-1]))
	    id[--n] = '\0';

	if (!*id)
	{
	    debug_print("%s: line %lu: ID expected", filename, line);
	    continue;
	}

	do
	    ++tp;
	while (isspace(*tp));

	if (!*tp)
	{
	    debug_print("%s: line %lu: Vendor name expected", filename, line);
	    continue;
	}

	name = tp;

	/*
	 * Find a place to put this new vendor.
	 * We will attach it to the end of the list.
	 */

	if (!(vp = (scsi_vend_t *)malloc(sizeof(scsi_vend_t))))
	{
	    err = ENOMEM;
	    break;
	}

	if (!(vp->id = strdup(id)))
	{
	    free(vp);
	    err = ENOMEM;
	    break;
	}

	if (!(vp->name = strdup(name)))
	{
	    free((void *)vp->id);
	    free(vp);
	    err = ENOMEM;
	    break;
	}

	vp->next = NULL;

	*vpp = vp;
	vpp = &(*vpp)->next;
    }

    fclose(fd);
    debug_print("%s: lines read %lu", filename, line);
    return errno = err;
}

