#ident	"@(#)modadmin:modadmin.c	1.3.1.5"
#ident	"$Header$"

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/errno.h>
#include	<pfmt.h>
#include	<locale.h>
#include	<time.h>
#include	<libgen.h>
#include	<sys/mod.h>

#define		LOAD		1
#define		UNLOAD		2
#define		STAT		3
#define		STATALL		4
#define		APATH		5
#define		DPATH		6

#define		CHECK_LIST()	if(optind == argc)	{ \
					(void)pfmt(stderr, MM_ERROR, ":1017:incorrect usage\n"); \
					prusage(); \
					exit(1); \
				}

extern	int	errno;

static	void	prusage(), scerror();

static	struct	modstatus		ms;
static	struct	modspecific_stat	*mss;

main(int argc, char **argv)
{
	extern	char	*optarg;
	extern	int	optind;

	int	opt_flag = 0;
	int	by_name = 0;
	int	verbose = 0;
	int	nopt = 0;
	int	err = 0;
	int	ok = 0;
	int	rv;
	int	mid;
	int	c;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:modadmin");

	while((c = getopt(argc, argv, "DdlQqSsUu")) != EOF)	{
		switch(c)	{
			case 'D':
				opt_flag = DPATH;
				break;

			case 'd':
				opt_flag = APATH;
				break;

			case 'l':
				opt_flag = LOAD;
				break;

			case 'Q':
				by_name = 1;

			case 'q':
				opt_flag = STAT;
				break;

			case 'S':
				verbose = 1;

			case 's':
				opt_flag = STATALL;
				break;

			case 'U':
				by_name = 1;

			case 'u':
				opt_flag = UNLOAD;
				break;

			case '?':
			default:
				nopt--;
				break;
		}
		nopt++;
	}

	if(!opt_flag || nopt > 1)	{
		(void)pfmt(stderr, MM_ERROR, ":1018:incorrect usage\n");
		prusage();
		exit(1);
	}

	switch(opt_flag)	{
		case LOAD:
			CHECK_LIST();
			while(optind < argc)	{
				if((mid = modload(argv[optind])) < 0)	{
					scerror(argv[optind]);
					err++;
				}
				else	{
					(void)pfmt(stderr, MM_INFO, ":1019:Module %s loaded, ID = %d\n",
						argv[optind], mid);
					ok++;
				}
				optind++;
			}
			break;

		case UNLOAD:
			CHECK_LIST();
			while(optind < argc)	{
				if((mid = get_mid(argv[optind], by_name)) >= 0)	{
					if(moduload(mid) < 0)	{
						scerror(argv[optind]);
						err++;
					}
					else	{
						if(mid)	{
							(void)pfmt(stderr, MM_INFO, ":1020:Module %d unloaded\n", mid);
						}
						else	{
							(void)pfmt(stderr, MM_INFO, ":1021:Modules unloaded\n", mid);
						}
						ok++;
					}
				}
				else	{
					err++;
				}
				optind++;
			}
			break;

		case STAT:
			CHECK_LIST();
			while(optind < argc)	{
				if((mid = get_mid(argv[optind], by_name)) >= 0)	{
					rv = prmodstat(&mid, 1, 0);
					if(rv)	err++;
					else	ok++;
				}
				else	{
					err++;
				}
				optind++;
			}
			break;

		case STATALL:
			if(optind != argc)	{
				(void)pfmt(stderr, MM_ERROR, ":1022:extra arguments given\n");
				prusage();
				exit(2);
				break;
			}
			mid = 1;
			while((rv = prmodstat(&mid, verbose, 1)) == 0)
				mid++;
			if(rv && rv != EINVAL)	{
				err = 1;
			}
			else	{
				ok++;
			}
			break;

		case APATH:
			CHECK_LIST();
			while(optind < argc)	{
				if(modpath(argv[optind]) < 0)	{
					scerror("modpath");
					err++;
				}
				else	{
					ok++;
				}
				optind++;
			}
			break;

		case DPATH:
			if(optind != argc)	{
				(void)pfmt(stderr, MM_ERROR, ":1023:extra arguments given\n");
				prusage();
				exit(2);
			}
			if(modpath(NULL) < 0)	{
				scerror("modpath");
				err++;
			}
			else	{
				(void)pfmt(stderr, MM_INFO, ":1024:Path reset to default\n");
				ok++;
			}
			break;

		default:
			break;
	}

	exit(err ? (ok ? 2 : 4) : 0);
}

static	void
prusage()
{
	(void)pfmt(stderr, MM_ACTION, ":1025:Usage: modadmin [-l string ...]\n");
	(void)pfmt(stderr, MM_NOSTD,  ":1026:\t\t\t    modadmin [-u objid ...]\n");
	(void)pfmt(stderr, MM_NOSTD,  ":1027:\t\t\t    modadmin [-U string ...]\n");
	(void)pfmt(stderr, MM_NOSTD,  ":1028:\t\t\t    modadmin [-q objid ...]\n");
	(void)pfmt(stderr, MM_NOSTD,  ":1029:\t\t\t    modadmin [-Q string ...]\n");
	(void)pfmt(stderr, MM_NOSTD,  ":1030:\t\t\t    modadmin [-s | -S]\n");
	(void)pfmt(stderr, MM_NOSTD,  ":1031:\t\t\t    modadmin [[-d dirname ...] | -D]\n");
}

static	void
scerror(const char *s)
{
	int	e;

	e = errno;

	if(s)	{
		(void)pfmt(stderr, MM_ERROR|MM_NOGET, "%s: %s\n", s, strerror(e));
	}
	else	{
		(void)pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(e));
	}

	if(e == ENOSYS)	{
		exit(3);
	}
}

static	int
get_mid(char *p, int by_name)
{
	char	*e;
	long	v;

	if(by_name)	{
		int	id = 1;

		while(modstat(id, &ms, 1) >= 0)	{
			if(strcmp(p, basename(ms.ms_path)) == 0)	{
				return(ms.ms_id);
			}
			id = ms.ms_id + 1;
		}
		if (errno == EPERM)
			(void)pfmt(stderr, MM_ERROR, ":1033:Not privileged\n");
		else
			(void)pfmt(stderr, MM_ERROR, ":1034:Module: %s, not found\n", p);
		return(-1);
	}

	v = strtol(p, &e, 10);

	if(e == p || *e != '\0')	{
		(void)pfmt(stderr, MM_ERROR, ":1035:Non-numeric ID string: %s\n", p);
		return(-1);
	}
	else	{
		return((int)v);
	}
}

static	int
prmodstat(int *id, int verbose, int get_next)
{
	int	nmaj, i;

	if(modstat(*id, &ms, get_next) < 0)	{
		if(get_next && errno == EINVAL)	{
			return(errno);
		}
		scerror("modstat");
		return(errno);
	}

	(void)pfmt(stdout, MM_NOSTD, ":1036:Module ID: %ld,\t\tModule: %s\n", ms.ms_id, ms.ms_path);

	*id = ms.ms_id;

	if(!verbose)	{
		return(0);
	}

	(void)pfmt(stdout, MM_NOSTD, ":1037:Size: %u,\t\tBase Address: 0x%x\n", ms.ms_size, ms.ms_base);
	(void)pfmt(stdout, MM_NOSTD, ":1038:Hold Count: %d,\tDependent Count: %d\n",
		ms.ms_holdcnt, ms.ms_depcnt);
	(void)pfmt(stdout, MM_NOSTD, ":1039:Unload Delay: %ld seconds\n", ms.ms_unload_delay);

	mss = ms.ms_msinfo;
	i = MODMAXLINK;

	while(i--)	{
		if(mss->mss_type == MOD_TY_NONE)	{
			break;
		}

		if(*mss->mss_linkinfo)	{
			(void)pfmt(stdout, MM_NOSTD|MM_NOGET, "%s\n", mss->mss_linkinfo);
		}

		switch(mss->mss_type)	{
			case MOD_TY_BDEV:
			case MOD_TY_CDEV:
			case MOD_TY_SDEV:
				(void)pfmt(stdout, MM_NOSTD, ":1040:Type:\tDevice Driver\n");

				if(nmaj = mss->mss_p0[1])	{
					if(nmaj == 1)	{
						(void)pfmt(stdout, MM_NOSTD, ":1041:\tBlock Major:\t\t%d\n",
								mss->mss_p0[0]);
					}
					else	{
						(void)pfmt(stdout, MM_NOSTD, ":1042:\tBlock Majors:\t\t%d - %d\n",
								mss->mss_p0[0],
								mss->mss_p0[0] + nmaj - 1);
					}
				}
				if(nmaj = mss->mss_p1[1])	{
					if(nmaj == 1)	{
						(void)pfmt(stdout, MM_NOSTD, ":1043:\tCharacter Major:\t%d\n",
								mss->mss_p1[0]);
					}
					else	{
						(void)pfmt(stdout, MM_NOSTD, ":1044:\tCharacter Majors:\t%d - %d\n",
								mss->mss_p1[0],
								mss->mss_p1[0] + nmaj - 1);
					}
				}
				break;

			case MOD_TY_FS:
				(void)pfmt(stdout, MM_NOSTD, ":1045:Type:\tFile System\n");
				(void)pfmt(stdout, MM_NOSTD, ":1046:\tSwitch Number:\t\t%d\n", mss->mss_p0[0]);
				break;

			case MOD_TY_STR:
				(void)pfmt(stdout, MM_NOSTD, ":1047:Type:\tSTREAMS Module\n");
				(void)pfmt(stdout, MM_NOSTD, ":1048:\tSwitch Number:\t\t%d\n", mss->mss_p0[0]);
				break;

			case MOD_TY_EXEC:
				(void)pfmt(stdout, MM_NOSTD, ":1032:Type:\tEXEC Module\n");
				(void)pfmt(stdout, MM_NOSTD, ":1049:\tMagic Number:\t\t%d\n", mss->mss_p0[0]);
				break;

			case MOD_TY_MISC:
				(void)pfmt(stdout, MM_NOSTD, ":1050:Type:\tMiscellaneous\n");
				break;

			default:
				(void)pfmt(stdout, MM_NOSTD, ":1051:Type:\tUNKNOWN\n");
				break;
		}
		fprintf(stdout, "\n");
		mss++;
	}

	return(0);
}
