#ident "@(#)builtin.c	1.7"
#ident "$Header$"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1995 Legent Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */
/*      SCCS IDENTIFICATION        */
#include <fcntl.h>
#include <sys/types.h>
#include <stropts.h>
#include <sys/stream.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/strioc.h>
#include <sys/dlpi.h>
#include <sys/scodlpi.h>

#include <sys/stat.h>
#include <sys/sysmacros.h>

#include <stdio.h>
#include <string.h>

#include "defs.h"
#include "proto.h"

/*
 * For systems that don't have persistent links, make do
 * with the regular ones
 */
#if !defined(I_PLINK)
#define	I_PLINK	I_LINK
#define PLINK_IS_LINK
#endif

extern	int	noexit;
extern	int	dounlink;

struct val      val_none = {V_NONE};
struct val      addr_none = {V_STR, ""};

extern int ksl_fd;
extern int Gflag;

static int target_count = 0;

/*
 * num - convert string to integer.  Returns 1 if ok, 0 otherwise. Result is
 * stored in *res.
 */
int
num(str, res, base)
	char           *str;
	int            *res;
	int		base;
{
	int             val;
	char           *p;

	val = strtol(str, &p, base);
	if (*p || p == str)
		return 0;
	else {
		*res = val;
		return 1;
	}
}

struct val     *
Open(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	static struct val rval = {V_FD};
	char *dev_major, *dev_minor, *comma;

	dev_major = dev_minor = NULL;

	if (comma = strchr(argv[0].u.sval, ',')) {
		if (!Gflag)
			xerr(fi, c, E_FATAL, "cannot specify major/minor to open for user-level slink");
		*comma = NULL;
		dev_major = argv[0].u.sval;
		dev_minor = comma+1;
		while (*dev_minor == ' ')
		  dev_minor++;
	}

	if (Gflag) {
		extern FILE *devfile;

		if (dev_major && dev_minor) {
		    	printf("\t__ksl_strcf_line = %d;\n", c->lineno);
			printf("\n\tksl_fds[%d] = ksl_open(%s, %s);\n",
				ksl_fd, dev_major, dev_minor);
			if (devfile) {
			  if (!isdigit(*dev_major))
			    fprintf(devfile, "extern %s\n", dev_major);
			  if (!isdigit(*dev_minor))
			    fprintf(devfile, "extern %s\n", dev_minor);
			}
		} else {
			char *varname;
			char *us;

			/*
			 * transform the file name into a variable name
			 * of the form `ksl_vars_path_name'.
			 */
			varname = (char *)malloc(strlen(argv[0].u.sval)
						 + 8 + 1);
			strcpy(varname, "ksl_vars");
			strcat(varname, argv[0].u.sval);

			us = varname;
			while (us = strchr(us, '/'))
			  *us = '_';

			printf("\n\tksl_fds[%d] = ksl_open_dev(%s);\n",
				ksl_fd, varname);

			if (devfile)
			  fprintf(devfile, "%s %s\n",
				  argv[0].u.sval, varname);
			free(varname);
		}
		rval.u.val = ksl_fd++;
		return &rval;
	}
	if ((rval.u.val = open(argv[0].u.sval, O_RDWR)) < 0)
		xerr(fi, c, E_SYS, "open \"%s\"", argv[0].u.sval);
	else {
	    nopen++;
	}
	return &rval;
}

static int      open_argtypes[] = {V_STR};
static struct bfunc open_info = {
	Open, 1, 1, open_argtypes
};

struct val *
Close(struct finst *fi, struct cmd *c, int argc, struct val *argv)
{
	if (Gflag) {
		printf("\t__ksl_strcf_line = %d;\n", c->lineno);
		printf("\tksl_close(ksl_fds[%d]);\n", argv[0].u.val);
	} else {
		close(argv[0].u.val);
		nopen--;
	}

	return &val_none;
}

static int close_argtypes[] = {V_FD};
static struct bfunc close_info = {
	Close, 1, 1, close_argtypes
};

struct val     *
Link(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	static struct val rval = {V_MUXID};

	if (dounlink == 0) {
		if (Gflag) {
		    	printf("\t__ksl_strcf_line = %d;\n", c->lineno);
			printf("\tksl_int = ksl_sioctl(ksl_fds[%d], 0x%lx, ksl_fds[%d]);\n",
				argv[0].u.val, I_LINK, argv[1].u.val);
			printf("\tksl_close(ksl_fds[%d]);\n", argv[1].u.val);
			rval.u.val = 0;
			return &rval;
		}

		if ((rval.u.val = 
			ioctl(argv[0].u.val, I_LINK, argv[1].u.val)) < 0)
			xerr(fi, c, E_SYS, "link");
	} else {
		/* tearing down, unlink everything since no MUX id available */
		(void) ioctl(argv[0].u.val, I_UNLINK, MUXID_ALL);
		rval.u.val = 0;
	}
	close(argv[1].u.val);
	nopen--;
	return &rval;
}

static int      link_argtypes[] = {V_FD, V_FD};
static struct bfunc link_info = {
	Link, 2, 2, link_argtypes
};

struct val     *
Plink(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	static struct val rval = {V_MUXID};

	if (dounlink == 0) {
		if (Gflag) {
		    	printf("\t__ksl_strcf_line = %d;\n", c->lineno);
			printf("\tksl_int = ksl_sioctl(ksl_fds[%d], 0x%lx, ksl_fds[%d]);\n",
				argv[0].u.val, I_PLINK, argv[1].u.val);
			printf("\tksl_close(ksl_fds[%d]);\n", argv[1].u.val);
			rval.u.val = 0;
			return &rval;
		}
		if ((rval.u.val = 
			ioctl(argv[0].u.val, I_PLINK, argv[1].u.val)) < 0)
		xerr(fi, c, E_SYS, "link");
	} else {
		/* tearing down, unlink everything since no MUX id available */
		(void) ioctl(argv[0].u.val, I_PUNLINK, MUXID_ALL);
		rval.u.val = 0;
	}
	close(argv[1].u.val);
	nopen--;
	return &rval;
}

static int      plink_argtypes[] = {V_FD, V_FD};
static struct bfunc plink_info = {
	Plink, 2, 2, plink_argtypes
};

struct val     *
Push(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	if (dounlink) {
		return &val_none;
	}
	if (Gflag) {
		struct stat sb;
		(void) stat(argv[0].u.sval, &sb);
	    	printf("\t__ksl_strcf_line = %d;\n", c->lineno);
		printf("\tksl_sioctl(ksl_fds[%d], 0x%lx, %s);\n",
				argv[0].u.val, I_PUSH, argv[1].u.sval);
		return &val_none;
	}
	if (ioctl(argv[0].u.val, I_PUSH, argv[1].u.sval) < 0)
		xerr(fi, c, E_SYS, "push \"%s\"", argv[1].u.sval);
	return &val_none;
}

static int      push_argtypes[] = {V_FD, V_STR};
static struct bfunc push_info = {
	Push, 2, 2, push_argtypes
};

/*
 * Vifname is similar to Sifname. The main difference is that it
 * does not require a muxid parameter. Vifname is used to push an
 * interface name down to the vmac driver used by Veritas Reliant HA
 */
struct val     *
Vifname(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	struct strioctl iocb;
	struct ifreq    ifr;
	uint	flags;

	if (dounlink) {
		return &val_none;
	}
	bzero(&ifr, sizeof(ifr));
	strcpy(ifr.ifr_name, argv[1].u.sval);
	if (!(num(argv[2].u.sval, (int *)&flags, 16)))
		xerr(fi, c, 0, "vifname: bad flags specification");
	ifr.ifr_metric = (flags << 16);
	if (Gflag) {
	    	printf("\t__ksl_strcf_line = %d;\n", c->lineno);
		printf("\t{\n\t\tstruct ifreq ksl_zz;\n");
		printf("\t\tbzero((caddr_t)&ksl_zz, sizeof(ksl_zz));\n");
		printf("\t\tkstrcpy(ksl_zz.ifr_name, \"%s\");\n",
			argv[1].u.sval);
		printf("\t\tksl_zz.ifr_metric = 0x%x | ksl_int;\n", 
			ifr.ifr_metric );
		printf("\t\tksl_ioctl(ksl_fds[%d], 0x%lx, (char *)&ksl_zz, sizeof(ksl_zz));\n",
			argv[0].u.val, SIOCSIFNAME);
		printf("\t}\n");
		return &val_none;
	}
	iocb.ic_cmd = SIOCSIFNAME;
	iocb.ic_timout = 15;
	iocb.ic_len = sizeof(ifr);
	iocb.ic_dp = (char *) &ifr;
	if (ioctl(argv[0].u.val, I_STR, &iocb) < 0)
		xerr(fi, c, E_SYS, "vifname");
	return &val_none;
}

static int      vifname_argtypes[] = {V_FD, V_STR, V_STR};
static struct bfunc vifname_info = {
	Vifname, 3, 3, vifname_argtypes
};

struct val     *
Sifname(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	struct strioctl iocb;
	struct ifreq    ifr;
	uint	flags;

	if (dounlink) {
		return &val_none;
	}
	bzero(&ifr, sizeof(ifr));
	strcpy(ifr.ifr_name, argv[2].u.sval);
	ifr.ifr_metric = argv[1].u.val;
	if (!(num(argv[3].u.sval, (int *)&flags, 16)))
		xerr(fi, c, 0, "sifname: bad flags specification");
	ifr.ifr_metric |= (flags << 16);
	if (Gflag) {
	    	printf("\t__ksl_strcf_line = %d;\n", c->lineno);
		printf("\t{\n\t\tstruct ifreq ksl_zz;\n");
		printf("\t\tbzero((caddr_t)&ksl_zz, sizeof(ksl_zz));\n");
		printf("\t\tkstrcpy(ksl_zz.ifr_name, \"%s\");\n",
			argv[2].u.sval);
		printf("\t\tksl_zz.ifr_metric = 0x%x | ksl_int;\n", 
			ifr.ifr_metric );
		printf("\t\tksl_ioctl(ksl_fds[%d], 0x%lx, (char *)&ksl_zz, sizeof(ksl_zz));\n",
			argv[0].u.val, SIOCSIFNAME);
		printf("\t}\n");
		return &val_none;
	}
	iocb.ic_cmd = SIOCSIFNAME;
	iocb.ic_timout = 15;
	iocb.ic_len = sizeof(ifr);
	iocb.ic_dp = (char *) &ifr;
	if (ioctl(argv[0].u.val, I_STR, &iocb) < 0)
		xerr(fi, c, E_SYS, "sifname");
	return &val_none;
}

static int      sifname_argtypes[] = {V_FD, V_MUXID, V_STR, V_STR};
static struct bfunc sifname_info = {
	Sifname, 4, 4, sifname_argtypes
};

struct val     *
Unitsel(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	struct strioctl iocb;
	int             unit;

	if (dounlink) {
		return &val_none;
	}
	if (!(num(argv[1].u.sval, &unit, 10)))
		xerr(fi, c, 0, "unitsel: bad unit number specification");
	if (Gflag) {
	    	printf("\t__ksl_strcf_line = %d;\n", c->lineno);
		printf("\t{ int ksl_zz = %d;\n",unit);
		printf("\t\tksl_ioctl(ksl_fds[%d], 0x%lx, &ksl_zz, sizeof(ksl_zz));\n",
			argv[0].u.val, IF_UNITSEL);
		printf("\t}\n");
		return &val_none;
	}
	iocb.ic_cmd = IF_UNITSEL;
	iocb.ic_timout = 0;
	iocb.ic_len = sizeof(int);
	iocb.ic_dp = (char *) &unit;
	if (ioctl(argv[0].u.val, I_STR, &iocb) < 0)
		xerr(fi, c, E_SYS, "unitsel");
	return &val_none;
}

static int      unitsel_argtypes[] = {V_FD, V_STR};
static struct bfunc unitsel_info = {
	Unitsel, 2, 2, unitsel_argtypes
};

struct val     *
Initqp(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	/* the order of these must agree with the IQP_XX defines */
	static char    *qname[] = {"rq", "wq", "hdrq", "muxrq", "muxwq"};
	static int      vtval[] = {IQP_LOWAT, IQP_HIWAT};
	static char    *vname[] = {"lowat", "hiwat"};
	struct iocqp    iocqp[IQP_NQTYPES * IQP_NVTYPES];
	int             niocqp;
	char           *dev;
	int             fd;
	struct strioctl iocb;
	int             i, qtype, vtype, val;

	if (dounlink) {
		return &val_none;
	}
	dev = argv[0].u.sval;
	niocqp = 0;
	for (i = 1; i < argc;) {
		for (qtype = 0; qtype < IQP_NQTYPES; qtype++) {
			if (strcmp(argv[i].u.sval, qname[qtype]) == 0)
				break;
		}
		if (qtype == IQP_NQTYPES) {
			xerr(fi, c, 0, "initqp: bad queue type \"%s\"",
			     argv[i].u.sval);
		}
		i++;
		if (i + IQP_NVTYPES > argc) {
			xerr(fi, c, 0, "initqp: incomplete specification for %s\n",
			     qname[qtype]);
		}
		for (vtype = 0; vtype < IQP_NVTYPES; vtype++, i++) {
			if (num(argv[i].u.sval, &val, 10)) {
				if (val < 0 || val > 65535) {
					xerr(fi, c, 0, "initqp: %s %s out of range",
					     qname[qtype], vname[vtype]);
				}
				iocqp[niocqp].iqp_type = qtype | vtval[vtype];
				iocqp[niocqp++].iqp_value = val;
			} else if (strcmp(argv[i].u.sval, "-")) {
				xerr(fi, c, 0, "initqp: illegal value for %s %s",
				     qname[qtype], vname[vtype]);
			}
		}
	}
	if (Gflag) {
		char *varname;
		char *us;
		extern FILE *devfile;

	    	printf("\t__ksl_strcf_line = %d;\n", c->lineno);
		printf("\t{\n\t\tstruct iocqp ksl_zz[%d];\n", niocqp);
		for (i = 0; i < niocqp; i++) {
			printf("\t\tksl_zz[%d].iqp_type = 0x%x;\n", i,
				iocqp[i].iqp_type);
			printf("\t\tksl_zz[%d].iqp_value = 0x%x;\n", i,
				iocqp[i].iqp_value);
		}
		printf("\n");

		/* transform the file name into a variable name
		 * of the form `ksl_vars_path_name'.
		 */
		varname = (char *)malloc(strlen(argv[0].u.sval)
					 + 8 + 1);
		strcpy(varname, "ksl_vars");
		strcat(varname, argv[0].u.sval);
			us = varname;
		while (us = strchr(us, '/'))
		  *us = '_';

		printf("\n\tksl_fds[%d] = ksl_open_dev(%s);\n",
			ksl_fd, varname);
		if (devfile)
		  fprintf(devfile, "%s %s\n",
				  argv[0].u.sval, varname);
		free(varname);

		printf("\t\tksl_ioctl(ksl_fds[%d], 0x%lx, (char *)ksl_zz, sizeof(ksl_zz));\n",
			ksl_fd, INITQPARMS);
		printf("\t\tksl_close(ksl_fds[%d]);\n",ksl_fd++);
		printf("\t}\n");
		return &val_none;
	}
	if ((fd = open(dev, O_RDWR)) < 0)
		xerr(fi, c, E_SYS, "initqp: open \"%s\"", dev);
	iocb.ic_cmd = INITQPARMS;
	iocb.ic_timout = 0;
	iocb.ic_len = niocqp * sizeof(struct iocqp);
	iocb.ic_dp = (char *) iocqp;
	if (ioctl(fd, I_STR, &iocb) < 0)
		xerr(fi, c, E_SYS, "initqp: ioctl INITQPARMS");
	close(fd);
	return &val_none;
}

static int      initqp_argtypes[] = {
	V_STR, V_STR, V_STR, V_STR, V_STR, V_STR, V_STR, V_STR,
	V_STR, V_STR, V_STR, V_STR, V_STR, V_STR, V_STR, V_STR
};
static struct bfunc initqp_info = {
	Initqp, 4, 16, initqp_argtypes
};

struct val     *
Dlattach(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	struct strbuf   ctlbuf;
	dl_attach_req_t att_req;
	dl_error_ack_t *error_ack;
	union DL_primitives dl_prim;
	int             flags = 0;
	int             fd, unit;

	if (dounlink) {
		return &val_none;
	}
	if (!(num(argv[1].u.sval, &unit, 10)))
		xerr(fi, c, 0, "dlattach: bad unit number specification");
	fd = argv[0].u.val;
	att_req.dl_primitive = DL_ATTACH_REQ;
	att_req.dl_ppa = unit;
	ctlbuf.len = sizeof(dl_attach_req_t);
	ctlbuf.buf = (char *) &att_req;
	if (putmsg(fd, &ctlbuf, NULL, 0) < 0)
		xerr(fi, c, E_SYS, "dlattach: putmsg");
	ctlbuf.maxlen = sizeof(union DL_primitives);
	ctlbuf.len = 0;
	ctlbuf.buf = (char *) &dl_prim;
	if (getmsg(fd, &ctlbuf, NULL, &flags) < 0)
		xerr(fi, c, E_SYS, "dlattach: getmsg");
	switch (dl_prim.dl_primitive) {
	case DL_OK_ACK:
		if (ctlbuf.len < sizeof(dl_ok_ack_t) ||
		    ((dl_ok_ack_t *) & dl_prim)->dl_correct_primitive
		    != DL_ATTACH_REQ)
			xerr(fi, c, 0, "dlattach: protocol error");
		else
			return &val_none;

	case DL_ERROR_ACK:
		if (ctlbuf.len < sizeof(dl_error_ack_t))
			xerr(fi, c, 0, "dlattach: protocol error");
		else {
			error_ack = (dl_error_ack_t *) & dl_prim;
			switch (error_ack->dl_errno) {
			case DL_BADPPA:
				xerr(fi, c, 0, "dlattach: bad PPA");

			case DL_ACCESS:
				xerr(fi, c, 0, "dlattach: access error");

			case DL_SYSERR:
				xerr(fi, c, 0, "dlattach: system error %d",
				     error_ack->dl_unix_errno);

			default:
				xerr(fi, c, 0, "dlattach: protocol error");
			}
		}

	default:
		xerr(fi, c, 0, "dlattach: protocol error");
	}
	/* NOTREACHED */
}

static int      dlattach_argtypes[] = {V_FD, V_STR};
static struct bfunc dlattach_info = {
	Dlattach, 2, 2, dlattach_argtypes
};

struct val     *
Strcat(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	static struct val rval = {V_STR};
	int             len;
	char           *newstr;

	len = strlen(argv[0].u.sval) + strlen(argv[1].u.sval) + 1;
	newstr = xmalloc(len);
	strcpy(newstr, argv[0].u.sval);
	strcat(newstr, argv[1].u.sval);
	rval.u.sval = newstr;
	return &rval;
}

static int      strcat_argtypes[] = {V_STR, V_STR};
static struct bfunc strcat_info = {
	Strcat, 2, 2, strcat_argtypes
};


struct val     *
Sifaddr(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	struct strioctl iocb;
	struct ifreq    ifr;
	int link;
	char enaddr[6];

	if (dounlink) {
		return &val_none;
	}
	strcpy(ifr.ifr_name, argv[1].u.sval);
	memcpy((caddr_t) ifr.ifr_enaddr, (caddr_t)argv[2].u.sval,
			sizeof(enaddr));

	if (Gflag) {
	    	printf("\t__ksl_strcf_line = %d;\n", c->lineno);
		printf("\t{\n\t\tstruct ifreq ksl_zz;\n");
		printf("\t\tbzero((caddr_t)&ksl_zz, sizeof(ksl_zz));\n");
		printf("\t\tkstrcpy(ksl_zz.ifr_name, \"%s\");\n",
			argv[1].u.sval);
		printf("\t\tbcopy(ksl_str, ksl_zz.ifr_enaddr, sizeof(ksl_zz.ifr_enaddr));\n");
		printf("\t\tksl_ioctl(ksl_fds[%d], 0x%lx, &ksl_zz, sizeof(ksl_zz));\n",
			argv[0].u.val, SIOCSIFNAME);
		printf("\t};\n");
		return &val_none;
	}
	iocb.ic_cmd = SIOCSIFNAME;
	iocb.ic_timout = 15;
	iocb.ic_len = sizeof(ifr);
	iocb.ic_dp = (char *) &ifr;
	if (ioctl(argv[0].u.val, I_STR, &iocb) < 0)
		xerr(fi, c, E_SYS, "sifaddr");

	return &val_none;
}

static int      sifaddr_argtypes[] = {V_FD, V_STR, V_STR};
static struct bfunc sifaddr_info = {
	Sifaddr, 3, 3, sifaddr_argtypes
};


struct val     *
Dlbind(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	struct strbuf   ctlbuf;
	dl_bind_req_t 	bind_req;
	dl_error_ack_t *error_ack;
	union DL_primitives dl_prim;
	union DL_primitives *info_dl_prim;
	int             flags = 0;
	int             fd, sap;
	int		dlpi = 0;
	static struct val rval = {V_STR};
	char           *newstr;

	if (dounlink) {
		return &addr_none;
	}
	if (!(num(argv[1].u.sval, &sap, 16)))
		xerr(fi, c, 0, "dlbind: bad sap number specification");
	fd = argv[0].u.val;

	if (Gflag) {
	    	printf("\t__ksl_strcf_line = %d;\n", c->lineno);
		printf("\tksl_dlbind(ksl_fds[%d], 0x%x, ksl_str);\n",
			fd, sap);
		return &addr_none;
	}

	bzero((char *) &bind_req, sizeof(bind_req));
	bind_req.dl_primitive = DL_BIND_REQ;
	bind_req.dl_sap = sap;
	bind_req.dl_service_mode = DL_CLDLS;
	ctlbuf.len = sizeof(dl_bind_req_t);
	ctlbuf.buf = (char *) &bind_req;
	ctlbuf.maxlen = 0;

	if (putmsg(fd, &ctlbuf, NULL, 0) < 0)
		xerr(fi, c, E_SYS, "dlbind: putmsg");

	bzero((char *)&dl_prim, sizeof(dl_prim));
	bzero((char *)&ctlbuf, sizeof(ctlbuf));
	ctlbuf.maxlen = sizeof(dl_prim);
	ctlbuf.len = 0;
	ctlbuf.buf = (char *) &dl_prim;

	if (getmsg(fd, &ctlbuf, NULL, &flags) < 0)
		xerr(fi, c, E_SYS, "dlbind: getmsg");

	switch (dl_prim.dl_primitive) {
	case DL_BIND_ACK:{
		dl_bind_ack_t *b_ack;

		b_ack = (dl_bind_ack_t *) &dl_prim;
		if (ctlbuf.len < sizeof(dl_bind_ack_t) ||
		    b_ack->dl_sap != sap) {
			xerr(fi, c, 0, "dlbind: protocol error");
		}
		else {	

			/* save the ethernet address */
			if(b_ack->dl_addr_length >0) {
				newstr = xmalloc(b_ack->dl_addr_length);
				memcpy((caddr_t) newstr, (caddr_t)&dl_prim +
					b_ack->dl_addr_offset,
					b_ack->dl_addr_length);
				rval.u.sval = newstr;
				return &rval;
			}
			return &addr_none;
		}
	}

	case DL_ERROR_ACK:
		if (ctlbuf.len < sizeof(dl_error_ack_t)) {
			xerr(fi, c, 0, "dlbind: protocol error");
		}
		else {
			error_ack = (dl_error_ack_t *) & dl_prim;
			switch (error_ack->dl_errno) {
			case DL_BADSAP:
				xerr(fi, c, 0, "dlbind: bad SAP");

			case DL_ACCESS:
				xerr(fi, c, 0, "dlbind: access error");

			case DL_SYSERR:
				xerr(fi, c, 0, "dlbind: system error %d",
				     error_ack->dl_unix_errno);

			default:
				xerr(fi, c, 0, "dlbind: protocol error %d",
				     error_ack->dl_errno);
			}
		}

	default:
		xerr(fi, c, 0, "dlbind: unexpected primitive %d",
			dl_prim.dl_primitive);
	}
	/* NOTREACHED */
}

static int      dlbind_argtypes[] = {V_FD, V_STR};
static struct bfunc dlbind_info = {
	Dlbind, 2, 2, dlbind_argtypes
};

struct val     *
Dlsubsbind(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	struct strbuf   ctlbuf;
	struct strbuf   databuf;
	dl_subs_bind_req_t *subs_bind_req;
	dl_error_ack_t *error_ack;
	union DL_primitives dl_prim;
	union DL_primitives data_prim;
	int             flags = 0;
	int             fd;
	static struct val rval = {V_STR};
	char *newptr;
	long dlsap;
	sco_snap_sap_t	*snap_sap;
	int size;

	if (dounlink) {
		return &val_none;
	}
	if (!(num(argv[1].u.sval, (int *) &dlsap,16)))
		xerr(fi, c, 0, "dlsubsbind: bad dlsap number specification");
	fd = argv[0].u.val;

	if (Gflag) {
	    	printf("\t__ksl_strcf_line = %d;\n", c->lineno);
		printf("\tksl_dlsubsbind(ksl_fds[%d], 0x%x);\n",
			fd, dlsap);
		return &val_none;
	}
	size = DL_SUBS_BIND_REQ_SIZE + sizeof(sco_snap_sap_t);

	newptr = xmalloc(size); 
	subs_bind_req = (dl_subs_bind_req_t *) newptr;
	subs_bind_req->dl_primitive = DL_SUBS_BIND_REQ;
	subs_bind_req->dl_subs_sap_offset = sizeof(dl_subs_bind_req_t);
	subs_bind_req->dl_subs_sap_length = sizeof(sco_snap_sap_t);
	subs_bind_req->dl_subs_bind_class = DL_HIERARCHICAL_BIND;
	snap_sap = (sco_snap_sap_t *) (newptr + subs_bind_req->dl_subs_sap_offset);
	snap_sap->prot_id =  0;
	snap_sap->type = dlsap;

	ctlbuf.buf = newptr;
	ctlbuf.len = size;
	ctlbuf.maxlen = 0;

	if (putmsg(fd, &ctlbuf, NULL, 0) < 0)
		xerr(fi, c, E_SYS, "dlsubsbind: putmsg");

	/*
	 * The following is necessary because of the fact that the DLPI
	 * driver returns a M_DATA message instead of a M_PCPROTO message.
	 * The below situation should be corrected once the DLPI driver
	 * is fixed.
	 */
	bzero((char *) &dl_prim, sizeof(dl_prim));
	bzero((char *) &data_prim, sizeof(dl_prim));

	ctlbuf.buf = (char *) &dl_prim;
	ctlbuf.maxlen = sizeof(dl_prim);
	ctlbuf.len = 0;

	databuf.buf = (char *) &data_prim;
	databuf.maxlen = sizeof(data_prim);
	databuf.len = 0;

	if (getmsg(fd, &ctlbuf, &databuf, &flags) < 0)
		xerr(fi, c, E_SYS, "dlsubsbind: getmsg");

	switch (dl_prim.dl_primitive) {
		case DL_SUBS_BIND_ACK:{
			dl_subs_bind_ack_t *subsb_ack;
			long dlsap;

			subsb_ack = (dl_subs_bind_ack_t *) &data_prim;
			if (ctlbuf.len < sizeof(dl_subs_bind_ack_t))  {
				xerr(fi, c, 0, "dlsubsbind: ack too small");
			} else{
				return &val_none;
			}
		}

		case DL_ERROR_ACK:
			if (ctlbuf.len < sizeof(dl_error_ack_t)) {
				xerr(fi, c, 0, "dlsubsbind: error-ack too small");
			} else {
				error_ack = (dl_error_ack_t *) & data_prim;
				switch (error_ack->dl_errno) {
				case DL_BADSAP:
					xerr(fi, c, 0, "dlsubsbind: bad DLSAP");
	
				case DL_ACCESS:
					xerr(fi, c, 0, "dlsubsbind: access error");

				case DL_SYSERR:
					xerr(fi, c, 0, "dlsubsbind: system error %d",
					     error_ack->dl_unix_errno);
				case DL_BADADDR:
					xerr(fi, c, 0, "dlsubsbind: bad addr %d",
					     error_ack->dl_unix_errno);

				default:
					xerr(fi, c, 0, "dlsubsbind: unexpected error code (%d,%x)", error_ack->dl_errno,error_ack->dl_errno);
				}
			}

		default:
			xerr(fi, c, 0, "dlsubsbind: unexpected primitive, (%d, %x)",
				dl_prim.dl_primitive, dl_prim.dl_primitive);
	}
		/* NOTREACHED */
}

static int      dlsubsbind_argtypes[] = {V_FD, V_STR};
static struct bfunc dlsubsbind_info = {
	Dlsubsbind, 2, 2, dlsubsbind_argtypes
};

struct val     *
Noexit(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	if (Gflag) {
	    	printf("\t__ksl_strcf_line = %d;\n", c->lineno);
		printf("\tksl_panicflag = 0;\n");
		return &val_none;
	}
	noexit = 1;
	return &val_none;
}

static struct bfunc noexit_info = {
	Noexit, 0, 0, NULL
};

struct val     *
Exit(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	if (Gflag) {
	    	printf("\t__ksl_strcf_line = %d;\n", c->lineno);
		printf("\tksl_panicflag = 1;\n");
		return &val_none;
	}
	noexit = 0;
	return &val_none;
}

static struct bfunc exit_info = {
	Exit, 0, 0, NULL
};

struct val     *
Sifhdr(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	struct strioctl iocb;
	int             type;

	if (dounlink) {
		return &val_none;
	}
	if (strcmp(argv[1].u.sval,"ieee")==0)
		type = ARPHRD_IEEE;
	else{
		if (strcmp(argv[1].u.sval,"ether")==0)
			type = ARPHRD_ETHER;
		else
			xerr(fi, c, E_SYS, "Sifhrd bad type: \"%s\"", 
					argv[1].u.sval);
	}

	if (Gflag) {
	    	printf("\t__ksl_strcf_line = %d;\n", c->lineno);
		printf("\t{ int ksl_zz = %d;\n", type);
		printf("\t\tksl_ioctl(ksl_fds[%d], 0x%lx, &ksl_zz, sizeof(ksl_zz));\n",
			argv[0].u.val, SIOCSHRDTYPE);
		printf("\t};\n");
		return &val_none;
	}

	iocb.ic_cmd = SIOCSHRDTYPE;
	iocb.ic_timout = 15;
	iocb.ic_len = sizeof(int);
	iocb.ic_dp = (char *) &type;
	if (ioctl(argv[0].u.val, I_STR, &iocb) < 0)
		xerr(fi, c, E_SYS, "sifhdr");
	return &val_none;
}

static int      sifhdr_argtypes[] = {V_FD, V_STR};
static struct bfunc sifhdr_info = {
	Sifhdr, 2, 2, sifhdr_argtypes
};

struct val     *
Addmcaddr(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	struct strioctl iocb;
	int             type;

	if (dounlink) {
		return &val_none;
	}

	if (Gflag) {
		char pp[9];
		int i;

		sscanf(argv[1].u.sval, "%02x:%02x:%02x:%02x:%02x:%02x",
			&pp[0], &pp[1], &pp[2], &pp[3], &pp[4], &pp[5]);
	    	printf("\t__ksl_strcf_line = %d;\n", c->lineno);
		printf("\t{\n\t\tchar ksl_zz[6];\n");
		for (i = 0; i < 6; i++) {
			printf("\t\tksl_zz[%d] = 0x%02x;\n",
				i, pp[i] & 0xff);
		}
		printf("\t\tksl_multi(ksl_fds[%d], 0x%lx, ksl_zz, sizeof(ksl_zz));\n",
			argv[0].u.val, DL_ENABMULTI_REQ);
		printf("\t};\n");
		return &val_none;
	}

	/* XXX should add real code here */
	return &val_none;
}

static int      addmcaddr_argtypes[] = {V_FD, V_STR};
static struct bfunc addmcaddr_info = {
	Addmcaddr, 2, 2, addmcaddr_argtypes
};

struct val     *
Call(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	if (Gflag) {
	    	printf("\t__ksl_strcf_line = %d;\n", c->lineno);
		printf("\t%s();\n", argv[0].u.sval);
	}
		
	return &val_none;
}

static int	call_argtypes[] = {V_STR};
static struct	bfunc call_info = {
	Call, 1, 1, call_argtypes
};

struct val *
Onerror(struct finst *fi, struct cmd *c, int argc, struct val *argv)
{
	if (Gflag) {
		printf("\t__ksl_strcf_line = %d;\n", c->lineno);
		printf("\t{\n\t\tstatic label_t ksl_%s_label_t;\n\n", argv[0].u.sval);
		printf("\t\tif (setjmp(ksl_%s_label_t) == 0) {\n", argv[0].u.sval);
		printf("\t\t\tksl_trampoline = ksl_%s_label_t;\n", argv[0].u.sval);
		printf("\t\t} else {\n");
		printf("\t\t\tgoto ksl_target_%s_%d;\n\t\t}\n\t}\n",
		       argv[0].u.sval, target_count);
	}
	return &val_none;
}

static int onerror_argtypes[] = {V_STR};
static struct bfunc onerror_info = {
	Onerror, 1, 1, onerror_argtypes
};

struct val *
Label(struct finst *fi, struct cmd *c, int argc, struct val *argv)
{
	if (Gflag) {
		printf("\t__ksl_strcf_line = %d;\n", c->lineno);
		printf("\tksl_target_%s_%d:\n", argv[0].u.sval, target_count++);
		printf("\tksl_trampoline = 0;\n");
	}
	return &val_none;
}

static int label_argtypes[] = {V_STR};
static struct bfunc label_info = {
	Label, 1, 1, label_argtypes
};

struct val     *
binit()
{
	deffunc("return", F_RETURN);
	deffunc("open", F_BUILTIN, &open_info);
	deffunc("close", F_BUILTIN, &close_info);
	deffunc("link", F_BUILTIN, &link_info);
	deffunc("plink", F_BUILTIN, &plink_info);
	deffunc("new_link", F_BUILTIN, &plink_info);
	deffunc("push", F_BUILTIN, &push_info);
	deffunc("sifname", F_BUILTIN, &sifname_info);
	deffunc("vifname", F_BUILTIN, &vifname_info);
	deffunc("sifaddr", F_BUILTIN, &sifaddr_info);
	deffunc("unitsel", F_BUILTIN, &unitsel_info);
	deffunc("dlattach", F_BUILTIN, &dlattach_info);
	deffunc("strcat", F_BUILTIN, &strcat_info);
	deffunc("initqp", F_BUILTIN, &initqp_info);
	deffunc("dlbind", F_BUILTIN, &dlbind_info);
	deffunc("dlsubsbind", F_BUILTIN, &dlsubsbind_info);
	deffunc("noexit", F_BUILTIN, &noexit_info);
	deffunc("exit", F_BUILTIN, &exit_info);
	deffunc("sifhrd", F_BUILTIN, &sifhdr_info);
	deffunc("addmcaddr", F_BUILTIN, &addmcaddr_info);
	deffunc("call", F_BUILTIN, &call_info);
	deffunc("onerror", F_BUILTIN, &onerror_info);
	deffunc("label", F_BUILTIN, &label_info);
}
