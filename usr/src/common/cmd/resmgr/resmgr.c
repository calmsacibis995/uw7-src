#ident	"@(#)resmgr:resmgr.c	1.5.6.1"

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<pfmt.h>
#include	<libgen.h>
#include	<locale.h>
#include	<string.h>
#include	<sys/file.h>
#include	<sys/types.h>
#include	<sys/resmgr.h>
#include	<sys/confmgr.h>
#include	<sys/cm_i386at.h>

#ifndef	CM_SCLASSID
#define	CM_SCLASSID	"SCLASSID"
#endif

#define	VB_SIZE	512

#define	AOPT	0x01	/* -a option mask bit */
#define	ROPT	0x02	/* -r option mask bit */
#define	IOPT	0x04	/* -i option mask bit */
#define	KOPT	0x08	/* -k option mask bit */
#define	MOPT	0x10	/* -m option mask bit */
#define POPT	0x20	/* -P option mask bit */
#define	VOPT	0x40	/* -v option mask bit */
#define	FOPT	0x80	/* -f option mask bit */
#define _CM_TYPE		0
#define _CM_MODNAME		1
#define _CM_UNIT		2
#define _CM_IPL			3
#define _CM_ITYPE		4
#define _CM_IRQ			5
#define _CM_IOADDR		6
#define _CM_MEMADDR		7
#define _CM_DMAC		8
#define _CM_BINDCPU		9
#define _CM_BRDBUSTYPE		10
#define _CM_SLOT		11
#define _CM_BRDID		12
#define _CM_CA_DEVCONFIG	13
#define _CM_ENTRYTYPE		14
#define _CM_CLAIM		15
#define _CM_BUSNUM		16
#define _CM_FUNCNUM		17
#define _CM_DEVNUM		18
#define _CM_SBRDID		19
#define	_CM_SCLASSID		20

#define	USAGE1	":976:\nusage:	%s\n"
#define	USAGE2	":977:\t%s -m modname [-p \"param1[ ...]\"]\n"
#define	USAGE3	":978:\t%s -k key [-p \"param1[ ...]\"]\n"
#define	USAGE4	":979:\t%s -p \"param1[ ...]\"\n\n"
#define	USAGE5	":980:\t%s -a -p \"param1[ ...]\" -v \"val1[ ...]\" [-d delim] [-i brdinst]\n\n"
#define	USAGE6	":981:\t%s -m modname -p \"param1[ ...]\" -v \"val1[ ...]\" [-d delim] [-i brdinst]\n"
#define	USAGE7	":982:\t%s -k key     -p \"param1[ ...]\" -v \"val1[ ...]\" [-d delim] [-i brdinst]\n\n"
#define	USAGE8	":983:\t%s -r -m modname [-i brdinst]\n"
#define	USAGE9	":984:\t%s -r -k key\n"
#define	USAGE10	":996:\t%s -f fname (i.e. /stand/resmgr)\n"

#define	NOMODN	":985:modname not allowed\n"
#define	MALLOC	":986:malloc() failed, size=%d \n"
#define NOOPEN  ":987:RMopen() failed, errno=%d\n"
#define GETBDK  ":988:RMgetbrdkey(%s, %d) failed, errno=%d\n"
#define DELKEY  ":989:RMdelkey(%d) failed, errno=%d\n"
#define GETVAL  ":990:RMgetvals(%d, %s, %d) failed, errno=%d\n"
#define PUTVAL  ":991:RMputvals(%d, %s, %s) failed, errno=%d\n"
#define DELVAL  ":992:RMdelvals(%d, %s) failed, errno=%d\n"
#define NEXTKY  ":993:RMnextkey(%d) failed, errno=%d\n"
#define R_OPEN  ":997:open( %s ) failed\n"
#define R_READK ":998:reading key failed\n"
#define R_READP ":999:reading param failed\n"
#define R_READV ":1000:reading value failed\n"
#define R_READL ":1001:reading value length failed\n"
#define R_KEY   ":1002:\nKey: %d\n"
#define R_UNK   ":1003: type of value unknown, try od -c fname\n"

#define	LAB_LEN	25	/* Max chars for setlabel() */

extern	char	*optarg;
extern	int	optind,
		opterr,
		optopt;

char		*modname	= NULL;
char		*param_list	= NULL;
char		*fname		= NULL;
char		*pl		= NULL;
char		*param		= NULL;
char		*val_list	= NULL;
char		*vl		= NULL;
char		*val		= NULL;
char		delim		= ' ';

int		brdinst		= 0;
int		optmask		= 0;
int		mode; /* RM_READ or RM_RDWR */

rm_key_t 	key		= 0;

char		val_buf[VB_SIZE];
char		D_buf[VB_SIZE] = "MODNAME UNIT IPL ITYPE IRQ IOADDR MEMADDR DMAC BINDCPU BRDBUSTYPE BRDID SCLASSID SLOT ENTRYTYPE BUSNUM";

void 		usage();
extern void	prrm();

char	*cmdname;
char	label[LAB_LEN+1] = "UX:";

main(int argc, char **argv)
{
	rm_key_t	rmkey;
	int		c, ec;
	int		Dump_flg = 0;
	cmdname = basename(argv[0]);

	/* Initialize locale information */
	(void)setlocale(LC_ALL, "");

	/* Initialize message label */
	(void)strncpy(&label[3], cmdname, LAB_LEN-3);
	(void)setlabel(label);

	/* Initialize catalog */
	(void)setcat("uxcore");

	while((c = getopt(argc, argv, "ari:k:m:p:v:d:f:")) != EOF) {
		switch(c) {
			case 'a':	/* add */
				if(optmask & (AOPT|ROPT|MOPT|KOPT|FOPT)) 
					usage();
				optmask |= AOPT;
				break;

			case 'r':	/* remove */
				if(optmask & (ROPT|AOPT|POPT|VOPT|FOPT)) 
					usage();
				optmask |= ROPT;
				break;

			case 'i':	/* instance */
				if(optmask & (IOPT|FOPT)) 
					usage();
				brdinst = atoi(optarg);
				optmask |= IOPT;
				break;

			case 'k':	/* key */
				if(optmask & (KOPT|AOPT|FOPT)) 
					usage();
				key = atoi(optarg);
				optmask |= KOPT;
				break;

			case 'm':	/* modname */
				if(optmask & (MOPT|KOPT|AOPT|FOPT)) 
					usage();
				modname = optarg;
				optmask |= MOPT;
				break;

			case 'p':       /* parameter */
				if(optmask & (POPT|ROPT|FOPT))
					usage();
				param_list = optarg;
				optmask |= POPT;
				break;

			case 'v':	/* value */
				if(optmask & (VOPT|ROPT|FOPT)) 
					usage();
				val_list = optarg;
				optmask |= VOPT;
				break;

			case 'd':	/* delimeter */
				if(optmask & (ROPT|FOPT))
					usage();
				delim = *optarg;
				break;

			case 'f':	/* Dump /stand/resmgr */
				if ( optmask != 0 )
					usage();
				fname = optarg;
				optmask = FOPT;
				break;
			default:
				usage();
		}
	}

	/* The command line is invalid if it contains an argument */
	if(optind < argc)
		usage();

	if ( optmask == FOPT ) {
		prrm( fname );
		/* We never return */
	}

	if(optmask & AOPT) {
		if(!(optmask & POPT) && !(optmask & VOPT))
			usage();
	}

	if(optmask & ROPT) {
		if((!(optmask & MOPT) && !(optmask & KOPT)))
		 	usage();
	}

	if(!(optmask & VOPT) && !(optmask & ROPT) && !(optmask & KOPT) && !(optmask & IOPT))
		Dump_flg = 1;

	if(argc == 1) {
		Dump_flg = 1;
		param_list = D_buf;
	} else {
		if(modname != NULL && optmask & AOPT) {
			(void)pfmt(stderr, MM_ERROR, NOMODN);
			usage();
		}
		if(param_list == NULL)
			param_list = D_buf;
	}

	if ((optmask & AOPT) || (optmask & VOPT) || (optmask & ROPT))
		mode= O_RDWR;
	else
		mode= O_RDONLY;
	if((ec = RMopen(mode)) != 0) {
		(void)pfmt(stderr, MM_ERROR, NOOPEN, ec);
		exit(1);
	}

	if(optmask & ROPT) {
		if(optmask & MOPT) {
			RMbegin_trans(key, RM_READ);
			if((ec = RMgetbrdkey(modname, brdinst, &rmkey)) != 0) {
				(void)pfmt(stderr, MM_ERROR, GETBDK,
					modname, brdinst, ec);
				RMend_trans(key);
				exit(1);
			}
			RMend_trans(key);
			key = rmkey;
		}
		ec = RMdelkey(key);
		if (ec != 0)
			(void)pfmt(stderr, MM_ERROR, DELKEY, key, ec);
		exit(ec);
	}

	if(Dump_flg) {
		rmkey = NULL;

		if(argc == 1 || optmask & KOPT) {
			(void)fprintf(stdout, "KEY %s\n", param_list);
		} else if(optmask & MOPT) {
			(void)fprintf(stdout, "MODNAME %s\n", param_list);
			if((pl = malloc(VB_SIZE+1)) == NULL) {
				(void)pfmt(stderr, MM_ERROR, MALLOC,
					strlen(val_buf)+1);
                		exit(1);
        		}
			memset(pl, '\0', (VB_SIZE+1));
	        	strcpy(pl, "MODNAME ");
	        	strcat(pl, param_list);
	        	strcpy(param_list, pl);
		} else
			(void)fprintf(stdout, "%s\n", param_list);

		RMbegin_trans(rmkey, RM_READ);
		while((ec = RMnextkey(&rmkey)) == 0) {
			if((ec = RMgetvals(rmkey, param_list, brdinst, val_buf,
				 VB_SIZE)) != 0) {
					(void)pfmt(stderr, MM_ERROR, GETVAL,
						rmkey, param_list, brdinst, ec);
					RMend_trans(rmkey);
					exit(1);
			}

			if(optmask & MOPT) {
				memset(pl, '\0', (VB_SIZE+1));
        			strcpy(pl, val_buf);
        			param = strtok(pl, " ");
				if(strcmp(param,modname) == 0) 
					(void)fprintf(stdout, "%s\n", val_buf);
			} else if(key) {
				if(key == rmkey) {
					(void)fprintf(stdout, "%d %s\n", rmkey,
						val_buf);
				}
			} else {
				if(argc == 1) {
					(void)fprintf(stdout, "%d %s\n", rmkey,
						val_buf);
				} else
					(void)fprintf(stdout, "%s\n", val_buf);
			}
		}
		RMend_trans(rmkey);
		free(pl);
		if(ec == ENOENT)
			exit(0);
		else {
			(void)pfmt(stderr, MM_ERROR, NEXTKY, rmkey, ec);
			exit(1);
		}
	}

	if(key) {
		rmkey = key;
	} else if(optmask & AOPT) {
		RMnewkey(&rmkey); /*RMnewkey does implicit begin_trans*/
		RMend_trans(key);
	} else {
		RMbegin_trans(rmkey, RM_READ);
		if((ec = RMgetbrdkey(modname, brdinst, &rmkey)) != 0) {
			(void)pfmt(stderr, MM_ERROR, GETBDK,
				modname, brdinst, ec);
			RMend_trans(rmkey);
			exit(1);
		}
		RMend_trans(rmkey);
	}

	if(val_list != NULL) {
		RMbegin_trans(rmkey, RM_RDWR);
		RMdelvals(rmkey, param_list);
		ec = RMend_trans(rmkey);
		if (ec<0){
			(void)pfmt(stderr, MM_ERROR, DELVAL,
				rmkey, param_list, ec);
			exit(1);
		}
		RMbegin_trans(rmkey, RM_RDWR);
		RMputvals_d(rmkey, param_list, val_list, delim);
		ec = RMend_trans(rmkey);
		if (ec<0){
			(void)pfmt(stderr, MM_ERROR, PUTVAL,
				rmkey, param_list, val_list, ec);
			exit(1);
		}
		exit(0);
	}

	RMbegin_trans(rmkey, RM_READ);
	if((ec = RMgetvals(rmkey, param_list, 0,  val_buf, VB_SIZE)) != 0) {
		(void)pfmt(stderr, MM_ERROR, GETVAL, rmkey, param_list,
			brdinst, ec);
		RMend_trans(rmkey);
		exit(1);
	}

	(void)fprintf(stdout, "%s\n", val_buf);
	RMend_trans(rmkey);
	exit(0);
}

void
usage()
{
	(void)pfmt(stderr, MM_ACTION, USAGE1, cmdname);
	(void)pfmt(stderr, MM_NOSTD, USAGE2, cmdname);
	(void)pfmt(stderr, MM_NOSTD, USAGE3, cmdname);
	(void)pfmt(stderr, MM_NOSTD, USAGE4, cmdname);
	(void)pfmt(stderr, MM_NOSTD, USAGE5, cmdname);
	(void)pfmt(stderr, MM_NOSTD, USAGE6, cmdname);
	(void)pfmt(stderr, MM_NOSTD, USAGE7, cmdname);
	(void)pfmt(stderr, MM_NOSTD, USAGE8, cmdname);
	(void)pfmt(stderr, MM_NOSTD, USAGE9, cmdname);
	(void)pfmt(stderr, MM_NOSTD, USAGE10, cmdname);
	exit(1);
}

int
param_to_tok( const char *param )
{
	static const char *cm_params[] = {
						CM_TYPE,
						CM_MODNAME,
						CM_UNIT,
						CM_IPL,
						CM_ITYPE,
						CM_IRQ,
						CM_IOADDR,
						CM_MEMADDR,
						CM_DMAC,
						CM_BINDCPU,
						CM_BRDBUSTYPE,
						CM_SLOT,
						CM_BRDID,
						CM_CA_DEVCONFIG,
						CM_ENTRYTYPE,
						CM_CLAIM,
						CM_BUSNUM,
						CM_FUNCNUM,
						CM_DEVNUM,
						CM_SBRDID,
						CM_SCLASSID,
						NULL
				   	 };

	int	i;

	for ( i = 0; cm_params[ i ] != NULL; i++ )
		if ( strcmp( cm_params[ i ], param ) == 0 )
			return i;

	return -1;
}

void
printval( int tok, const char *param, const char *val, size_t vlen, int first )
{
	if ( first == 0 )
		printf( "  %16s", "" );

	switch ( tok )
	{
		/* cm_num_t */

		case _CM_TYPE:
		case _CM_UNIT:
		case _CM_IPL:
		case _CM_ITYPE:
		case _CM_IRQ:
		case _CM_DMAC:
		case _CM_BINDCPU:
		case _CM_BRDBUSTYPE:
		case _CM_SLOT:
		case _CM_CA_DEVCONFIG:
		case _CM_ENTRYTYPE:
		case _CM_CLAIM:
		case _CM_BUSNUM:
		case _CM_FUNCNUM:
		case _CM_DEVNUM:

			printf( " 0x%X\n", *(cm_num_t *)val );
			break;

		/* cm_range_t */

		case _CM_IOADDR:
		case _CM_MEMADDR:

			printf( " 0x%X - 0x%X\n", ((cm_range_t *)val)->startaddr,
						((cm_range_t *)val)->endaddr );
			break;
			
		/* string */

		case _CM_BRDID:
		case _CM_MODNAME:
		case _CM_SBRDID:
		case _CM_SCLASSID:

			printf( "%*s\n", vlen, val );
			break;

		/* unknown */

		default:

			(void)pfmt( stdout, MM_NOSTD, R_UNK );
			break;
	}
}

void
prrm( const char *fname )
{
	rm_key_t	key;
	size_t		valbufsz = 32;
	size_t		vlen;
	char		param[ RM_MAXPARAMLEN ];
	char		*val;
	int		first;
	int		infd;
	int		tok;


	if ( fname == NULL )
	{
		usage();
		exit( 1 );
	}

	if (( infd = open( fname, O_RDONLY )) == -1 )
	{
		(void)pfmt(stderr, MM_ERROR, R_OPEN, fname );
		exit( 1 );
	}

	if (( val = (char *)malloc( valbufsz )) == NULL )
	{
		(void)pfmt(stderr, MM_ERROR, MALLOC, valbufsz );
		exit( 1 );
	}


	for ( ;; )
	{
		if ( read( infd, &key, sizeof( rm_key_t )) != sizeof( rm_key_t ))

		{
			(void)pfmt(stderr, MM_ERROR, R_READK );
			exit( 1 );
		}

		if ( key == RM_NULL_KEY )
			exit( 0 );

		(void)pfmt( stdout, MM_NOSTD, R_KEY, key );

		for ( ;; )
		{
			if ( read( infd, param, RM_MAXPARAMLEN ) != RM_MAXPARAMLEN )
			{
				(void)pfmt(stderr, MM_ERROR, R_READP );
				exit( 1 );
			}

			if ( strcmp( param, RM_ENDOFPARAMS ) == 0 )
				break;

			printf( "  %-16s", param );
			tok = param_to_tok( param );
			first = 1;

			for ( ;; )
			{
				if ( read( infd, &vlen, sizeof( size_t )) != sizeof( size_t ))
				{
					(void)pfmt(stderr, MM_ERROR, R_READL );
					exit( 1 );
				}

				if ( vlen == RM_ENDOFVALS )
					break;

				if ( vlen > valbufsz )
				{
					free( val );
					valbufsz = vlen;

					if (( val = (char *)malloc( valbufsz )) == NULL )
					{
						(void)pfmt(stderr, MM_ERROR,
								MALLOC, valbufsz );
						exit( 1 );
					}
				}

				if ( read( infd, val, vlen ) != vlen )
				{
					(void)pfmt(stderr, MM_ERROR, R_READV );
					exit( 1 );
				}

				printval( tok, param, val, vlen, first );
				first = 0;
			}
		}
	}
}
