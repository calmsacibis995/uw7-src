/*
 * File hw.c
 *
 * @(#) hw.c 67.1 97/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <pwd.h>

const char	*basename(const char *name);

#include "hw_buf.h"
#include "hw_bus.h"
#include "hw_cache.h"
#include "hw_con.h"
#include "hw_cpu.h"
#include "hw_eisa.h"
#include "hw_isa.h"
#include "hw_mp.h"
#include "hw_mca.h"
#include "hw_net.h"
#include "hw_pci.h"
#include "hw_pcmcia.h"
#include "hw_ram.h"
#include "hw_rom.h"
#include "hw_scsi.h"
#include "hw_util.h"

/*
 * The values of these is also the index into the array my_names.
 */

typedef enum
{
    hw_pers,
    yo_pers,
    wach_pers,
    yagot_pers
} pers_t;

static void bad_param(pers_t pers, FILE *out, int param);
static void example(pers_t pers, FILE *out, int err, const char *fmt, ...);
static void add_report(const char *report);
static void std_help(FILE *out, const char *name, const char *extra);
static const char *version(void);
static void show_hw_stuff(FILE *out);
static int  get_stuff_index(const char *stuff);

static int  hw_main(pers_t pers, FILE *out, int argc, const char *const *argv);
static void hw_help(FILE *out);
static int  yo_main(pers_t pers, FILE *out, int argc, const char *const *argv);
static void yo_help(FILE *out);
static int  wach_main(pers_t pers, FILE *out,int argc,const char *const *argv);
static void wach_help(FILE *out);
static int  yagot_main(pers_t pers, FILE *out,int argc,const char *const *argv);
static void yagot_help(FILE *out);

/*
 * This program can be called by several names
 */

typedef struct
{
    pers_t	pers;	/* Also the index into the array */
    const char	*name;
    int		(*main)(pers_t pers,FILE *out,int argc,const char *const *argv);
    void	(*main_help)(FILE *out);
    const char	*opt;
    int		sanity;
} my_names_t;

static const char	hw_name[] =	"hw";
static const char	yo_name[] =	"yo";
static const char	wach_name[] =	"wachagot";
static const char	yagot_name[] =	"yagot";

#define STD_OPTS	"Vhvmp:l:n:"

static const char	yo_opt[] = STD_OPTS "r:X";
static const char	hw_opt[] = STD_OPTS "r:s";

/*
 * The value of my_names[n].pers must be a valid index into the
 * array my_names.  Preferably pointing to itself.
 */

my_names_t	my_names[] =
{
    { hw_pers,	  hw_name,	hw_main,	hw_help,	hw_opt, 1 },
    { yo_pers,	  yo_name,	yo_main,	yo_help,	yo_opt, 0 },
    { wach_pers,  wach_name,	wach_main,	wach_help,	yo_opt, 0 },
    { yagot_pers, yagot_name,	yagot_main,	yagot_help,	yo_opt, 0 },
};

/*
 * Information is available on many resources
 */

typedef struct
{
    const char * const	*callme;
    const char		*short_help;
    int			(*have)(void);
    void		(*report)(FILE *out);
    int			multi;
} hw_stuff_t;

#define HW_STUFF(x,m)	\
    { callme_ ## x, short_help_ ## x, have_ ## x, report_ ## x, m }

/*
 * These are sorted in the order that the reports are generated
 * by wachagot.
 */

hw_stuff_t	hw_stuff[] =
{
    /*
     * Basic stuff
     */
    HW_STUFF(cpu,    0),
    HW_STUFF(ram,    0),
    HW_STUFF(rom,    0),
    HW_STUFF(mp,     0),
    HW_STUFF(con,    0),

    /*
     * CPU bus probes
     */
    HW_STUFF(bus,    1),	/* This is just a group of others */
    HW_STUFF(eisa,   0),
    HW_STUFF(isa,    0),
/*  HW_STUFF(mca,    0),	## Not Yet */
    HW_STUFF(pci,    0),
    HW_STUFF(pcmcia, 0),
/*  HW_STUFF(scsi,   0), */

    /*
     * More complex data
     */
/*  HW_STUFF(buf,    0),	## Not Yet */
/*  HW_STUFF(cache,  0),	## Not Yet */
/*  HW_STUFF(net,    0),	## Not Yet */
};

/*
 * Globals for hw only
 */

static const char	**report_list = NULL;

int	silent = 0;

/*
 * Misc data
 */

int	sanity = 1;

const char	*KernelName = NULL;

static const char	pager_env[] = "PAGER";
static const char	dflt_pager[] = "/usr/bin/more";

/*
 * Public Routines
 */

int
main(int argc, const char * const *argv)
{
    pers_t	pers;
    int		c;
    FILE	*out = stdout;
    const char	*pager;
    const char	*prog;
    const char	*opts;

    /*
     * Find out where we are.  We must look at our name before
     * parsing the command line so that we know what arguments
     * to expect.  If we cannot figure it out, be hw_pers.
     */

    prog = basename(argv[0]);

    for (c = 0; c < sizeof(my_names)/sizeof(*my_names); ++c)
	if (strcmp(prog, my_names[c].name) == 0)
	    break;

    if (c >= sizeof(my_names)/sizeof(*my_names))
	c = 0;	/* The default personality, hw_pers */

    pers = my_names[c].pers;
    sanity = my_names[pers].sanity;
    opts = my_names[pers].opt;

    /*
     * Find a pager
     */

    if (!(pager = getenv(pager_env)) || !*pager)
        pager = dflt_pager;

    /*
     * Parse the command line
     *
     * Do not do anything here that expects root.  We have not
     * yet checked the users credentials and we do not want to
     * until we determine that all we need is the help message.
     */

    while ((c = getopt(argc, (char **)argv, opts)) != -1)
	switch (c)
	{
	    /*
	     * General flags
	     */

	    case 'V':
		fprintf(stderr, "%s\n", version());
		return 0;

	    case 'h':
		example(pers, out, 0, NULL);

	    case 'v':
		verbose = 1;
		break;

	    case 'X':		/* Undocumented - Ugly */
		debug = 1;
		break;

	    case 'l':
		lib_dir = optarg;
		debug_print("lib_dir is: %s", lib_dir);
		break;

	    case 'p':
		pager = optarg;
		/* Fall thru */
	    case 'm':
		debug_print("Pager is: %s", pager);

		if (out != stdout)
		    pclose(out);
		if (!(out = popen(pager, "w")))
		{
		    fprintf(stderr, "Cannot open pager: %s\n", pager);
		    return errno;
		}
		errorFd = out;
		break;

	    case 'n':
		KernelName = optarg;
		break;

	    /*
	     * hw specific flags
	     * The -r option is allowed for yo but has no real use.
	     */

	    case 'r':
		if ((pers != hw_pers) && (pers != yo_pers))
		    bad_param(pers, out, c);

		add_report(optarg);
		break;

	    case 's':
		silent = 1;
		break;

	    default:
		bad_param(pers, out, optopt);
	}

    /*
     * Environment check
     */

    if (geteuid() != 0)
    {
	struct passwd	*pw;


	if (sanity)
	{
	    fprintf(stderr, "Permission denied\n");
	    return(EPERM);
	}

	if ((pw = getpwuid(geteuid())) != NULL)
	    fprintf(stderr, "Ya gotta be root, %s\n", pw->pw_name);
	else
	    fprintf(stderr, "Ya gotta be root\n");

	return(EPERM);
    }

    /*
     * Normal personality parsing
     */

    c = my_names[pers].main(pers, out, argc - optind, &argv[optind]);
    if (out != stdout)
	pclose(out);
    return c;
}

/*
 * Private Routines
 */

static void
bad_param(pers_t pers, FILE *out, int param)
{
    example(pers, out, EINVAL, "Unexpected parameter: %c", param & 0xff);
}

static void
example(pers_t pers, FILE *out, int err, const char *fmt, ...)
{
    fprintf(out, "\n");
    if (fmt && *fmt)
    {
	va_list	args;

	va_start(args, fmt);
	vfprintf(out, fmt, args);
	va_end(args);
	fprintf(out, "\n");
    }

    if (my_names[pers].main_help)
	my_names[pers].main_help(out);

    exit(err);
}

/*
 * The passed string "report" has the life of this program. 
 * There is no need to copy it.
 */

static void
add_report(const char *report)
{
    int		n;
    char const	**tp;

    if (report_list)
    {
	char const	**fp;

	for (n = 0, fp = report_list; *fp; ++fp)
	    ++n;

	tp = (char const **)Fmalloc(sizeof(char *) * (n+2));

	for (n = 0, fp = report_list; *fp; ++fp)
	    tp[n++] = *fp;

	tp[n++] = report;
	tp[n] = NULL;

	free((void *)report_list);
	report_list = tp;
    }
    else
    {
	report_list = tp = (char const **)Fmalloc(sizeof(char *) * 2);
	*tp++ = report;
	*tp = NULL;
    }
}

/*
 * These are the standard options.  If you change the standard
 * options, you should change this help and STD_OPTS.  Remember
 * that -X is not documented by design.
 */

static void
std_help(FILE *out, const char *name, const char *extra)
{
    fprintf(out,
	"%s   %s\n\n"
	"Use:\n"
	"    %s -V\n"
	"        Display version message\n"
	"    %s -h\n"
	"        Display this help message\n"
	"    %s [-v] [-l <lib_dir>] [-m] [-p <pager>] [-n <kernel>]",
		    name, version(),
		    name,
		    name,
		    name);

    if (extra && *extra)
	fprintf(out, " %s", extra);

    fprintf(out,
	"\n"
	"        -v           Verbose output\n"
	"        -l <lib_dir> Specify library directory\n"
	"                         default: $%s or .:.%s:%s\n"
	"        -m           Output processed by a pager\n"
	"        -p <pager>   Specify pager (implies -m)\n"
	"                         default: $%s or %s\n"
	"        -n <kernel>  Specify kernel\n"
	"                         default: search for it\n",
				    lib_dir_env, dflt_lib_dir, dflt_lib_dir,
				    pager_env, dflt_pager);
}

static const char *
version()
{
    extern char		LinkDate[];		/* This is in linkdate.c */
    static char		*verBuf = NULL;


    if (!verBuf)
    {
	verBuf = Fmalloc(256);
	strcpy(verBuf, "Version");
	strcat(verBuf, ":");
	strcat(verBuf, LinkDate);
    }

    return verBuf;
}

static void
show_hw_stuff(FILE *out)
{
    int		n;

    if (sanity)
	fprintf(out, "\nReports available:\n");
    else
	fprintf(out, "\nStuff I Think I know about:\n");

    fprintf(out, "    ");
    for (n = 0; n < sizeof(hw_stuff)/sizeof(*hw_stuff); ++n)
    {
	int	s;

	for (s = 0; hw_stuff[n].callme[s]; ++s)
	{
	    fprintf(out, "%s", hw_stuff[n].callme[s]);
	    if (hw_stuff[n].callme[s+1])
	        fprintf(out, ", ");
	}
	fprintf(out, "\n        %s\n", hw_stuff[n].short_help);
	if ((n+1) < sizeof(hw_stuff)/sizeof(*hw_stuff))
	    fprintf(out, "    ");
    }
}

static int
get_stuff_index(const char *stuff)
{
    int		n;

    for (n = 0; n < sizeof(hw_stuff)/sizeof(*hw_stuff); ++n)
    {
	int	s;

	for (s = 0; hw_stuff[n].callme[s]; ++s)
	    if (strcasecmp((char *)stuff, (char *)hw_stuff[n].callme[s]) == 0)
		return n;
    }

    return -1;
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				The hw Personality			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 *
 * For the wachagot functionality:
 *	hw
 *
 * For the yo functionality:
 *	hw -r <something>
 *
 * For the yagot functionality:
 *	hw -sr <something>
 */

static int
hw_main(pers_t pers, FILE *out, int argc, const char * const *argv)
{
    int		n;

    if (argc)
	bad_param(pers, out, **argv);

    if (silent)
    {
	/*
	 * Silent results in something like yagot
	 */

	if (report_list)
	{
	    /*
	     * With a report list results in something like yo
	     */

	    char const	**tp;

	    for (tp = report_list; *tp; ++tp)
	    {
		if ((n = get_stuff_index(*tp)) == -1)
		    return EINVAL;

		if (!hw_stuff[n].have())
		    return ENODEV;
	    }
	}
    }
    else if (report_list)
    {
	/*
	 * With a relort list results in something like yo
	 */

	char const	**tp;

	for (tp = report_list; *tp; ++tp)
	{
	    if ((n = get_stuff_index(*tp)) == -1)
		example(pers, out, EINVAL, "No such report: %s", *tp);

	    hw_stuff[n].report(out);
	}
    }
    else
    {
	/*
	 * No reports requested works something like wachagot
	 */

	for (n = 0; n < sizeof(hw_stuff)/sizeof(*hw_stuff); ++n)
	    if (hw_stuff[n].have() && !hw_stuff[n].multi)
	    {
		hw_stuff[n].report(out);
		fflush(out);
	    }
    }

    return 0;
}

static void
hw_help(FILE *out)
{
    std_help(out, hw_name, "[-r <report>] [-s]");
    fprintf(out,
	"        -r <report>  Specify report\n"
	"                         Note: multiple reports are supported\n"
	"        -s           Silent; return success or fail status\n");
    show_hw_stuff(out);
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				The yo Personality			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

static int
yo_main(pers_t pers, FILE *out, int argc, const char * const *argv)
{
    int		work = 0;

    while (argc--)
    {
	int	n;

	if ((n = get_stuff_index(*argv)) == -1)
	    example(pers, out, EINVAL, "I don't grok %s?", *argv);

	argv++;
	hw_stuff[n].report(out);
	fflush(out);
	++work;
    }

    if (!work)
	example(pers, out, 0, "What yo need?");

    return 0;
}

static void
yo_help(FILE *out)
{
    std_help(out, yo_name, "[-r] <stuff>");
    show_hw_stuff(out);
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				The wachagot Personality		 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

static int
wach_main(pers_t pers, FILE *out, int argc, const char * const *argv)
{
    int		n;

    if (argc)
	bad_param(pers, out, **argv);

    for (n = 0; n < sizeof(hw_stuff)/sizeof(*hw_stuff); ++n)
	if (hw_stuff[n].have() && !hw_stuff[n].multi)
	{
	    hw_stuff[n].report(out);
	    fflush(out);
	}

    return 0;
}

static void
wach_help(FILE *out)
{
    std_help(out, wach_name, NULL);
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				The yagot Personality			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

static int
yagot_main(pers_t pers, FILE *out, int argc, const char * const *argv)
{
    int		work = 0;
    int		have = 1;

    while (argc--)
    {
	int	n;

	if ((n = get_stuff_index(*argv)) == -1)
	    example(pers, out, EINVAL, "I don't grok %s?", *argv);

	argv++;
	if (!hw_stuff[n].have())
	    have = 0;
	++work;
    }

    if (!work)
	example(pers, out, 0, "What I got you need?");

    if (verbose)
    {
	if (have)
	    fprintf(out, "Sure do! :-)\n");
	else
	    fprintf(out, "Nope! :-(\n");
    }

    return have ? 0 : ENODEV;
}

static void
yagot_help(FILE *out)
{
    std_help(out, yagot_name, "<stuff>");
    show_hw_stuff(out);
}

