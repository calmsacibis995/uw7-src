/*		copyright	"%c%" 	*/

#ident	"@(#)acct:common/cmd/acct/bfsdiskusg.c	1.1.1.4"
#ident "$Header$"

/* bfsdiskusg:  diskusg for bfs */

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/fs/bfs.h>
#include <sys/sysmacros.h>
#include <pwd.h>
#include <fcntl.h>
#include "acctdef.h"

struct	bdsuper	sblock;

#define	INOCHNK	1024			/* # of inodes to read at once */
struct	bfs_dirent	dinode[INOCHNK];

int VERBOSE = 0;
FILE	*ufd = 0;		/* fd for file of unattributable files */
unsigned ino;

struct acct  {
	uid_t	uid;
	long	usage;
	char	name [MAXNAME+1];
} userlist [MAXUSERS];

char	*cmd;

unsigned hash();

main(argc, argv)
int argc;
char **argv;
{
	extern	int	optind;
	extern	char	*optarg;
	register c;
	register FILE	*fd;
	register	rfd;
	struct	stat	sb;
	int	sflg = {FALSE}, iflg = {FALSE};
	char 	*pfile = NULL;	/* password file name */
	int	errfl = {FALSE};

	cmd = argv[0];
	while((c = getopt(argc, argv, "vu:p:si:")) != EOF) switch(c) {
	case 's':
		sflg = TRUE;
		break;
	case 'v':
		VERBOSE = 1;
		break;
	case 'i':
		iflg = TRUE;
		errfl++;
		break;
	case 'u':
		ufd = fopen(optarg, "a");
		break;
	case 'p':
		pfile = optarg;
		break;
	case '?':
		errfl++;
		break;
	}
	if(errfl) {
		if(iflg)
			fprintf(stderr, "Option -i not supported by %s\n", cmd);
		else
			fprintf(stderr, "Usage: %s [-sv] [-p pw_file] [-u file] [-i ignlist] [file ...]\n", cmd);
		exit(10);
	}

	hashinit();
	if(sflg == TRUE) {
		if(optind == argc){
			adduser(stdin);
		} else {
			for( ; optind < argc; optind++) {
				if( (fd = fopen(argv[optind], "r")) == NULL) {
					fprintf(stderr, "%s: Cannot open %s\n", cmd, argv[optind]);
					continue;
				}
				adduser(fd);
				fclose(fd);
			}
		}
	}
	else {
		setup(pfile);
		for( ; optind < argc; optind++) {
			if( (rfd = open(argv[optind], O_RDONLY)) < 0) {
				fprintf(stderr, "%s: Cannot open %s\n", cmd, argv[optind]);
				continue;
			}
			if(fstat(rfd, &sb) >= 0){
				if ( (sb.st_mode & S_IFMT) == S_IFCHR ||
				     (sb.st_mode & S_IFMT) == S_IFBLK ) {
					ilist(argv[optind], rfd);
				} else {
					fprintf(stderr, "%s: %s is not a special file -- ignored\n", cmd, argv[optind]);
				}
			} else {
				fprintf(stderr, "%s: Cannot stat %s\n", cmd, argv[optind]);
			}
			close(rfd);
			if (ufd) fclose(ufd);
		}
	}
	output();
	exit(0);
}

adduser(fd)
register FILE	*fd;
{
	uid_t	usrid;
	long	blcks;
	char	login[MAXNAME+10];
	int	index;

	while(fscanf(fd, "%ld %s %ld\n", &usrid, login, &blcks) == 3) {
		if( (index = hash(usrid)) == FAIL) return(FAIL);
		if(userlist[index].uid == UNUSED) {
			userlist[index].uid = usrid;
			strncpy(userlist[index].name, login, MAXNAME);
		}
		userlist[index].usage += blcks;
	}
}

ilist(file, fd)
char	*file;
register fd;
{
	register dev_t	dev;
	register i, j;
	int	inopb, inoshift, fsinos, bsize;
	unsigned nfiles;		/* number of inodes in filesystem */

	if (fd < 0 ) {
		return (FAIL);
	}

	sync();

	/* Read in super-block of filesystem */
	bread(fd, BFS_SUPEROFF, &sblock, sizeof(sblock));

	/* Check if filesystem is really BFS */
	if (sblock.bdsup_bfsmagic != BFS_MAGIC ) {
		fprintf(stderr, "%s: %s not a ufs file system, ignored\n",
			cmd, file);
		return(-1);
	}

	inopb = BFS_BSIZE/sizeof(struct bfs_dirent);


	nfiles = (sblock.bdsup_start - BFS_DIRSTART)/sizeof(struct bfs_dirent);

	i = BFS_DIRSTART/BFS_BSIZE;
	for (ino = 0; ino < nfiles; i += INOCHNK/inopb) {
		bread(fd, i, dinode, sizeof(dinode));
		for (j = 0; j < INOCHNK && ino++ < nfiles; j++)
			if (dinode[j].d_fattr.va_mode & S_IFMT)
				if(count(j, dev) == FAIL) {
					if(VERBOSE)
						fprintf(stderr,"BAD UID: file system = %s, inode = %u, uid = %ld\n",
					    	file, ino, dinode[j].d_fattr.va_uid);
					if(ufd)
						fprintf(ufd, "%s %u %ld\n", file, ino, dinode[j].d_fattr.va_uid);
				}
	}
	return (0);
}

bread(fd, bno, buf, cnt)
register fd;			/* fs file descriptor */
register unsigned bno;		/* block number to read from */
register struct  bfs_dirent  *buf;	
register cnt;
{
	lseek(fd, (long)bno*BFS_BSIZE, 0);
	if (read(fd, buf, cnt) != cnt)
	{
		fprintf(stderr, "%s: read error %u\n", cmd, bno);
		close(fd);
		exit(1);
	}
}

count(j, dev)
register j;
register dev_t dev;
{
	int index;

	if ( dinode[j].d_fattr.va_nlink == 0 || dinode[j].d_fattr.va_mode == 0 )
		return(SUCCEED);
	if( (index = hash(dinode[j].d_fattr.va_uid)) == FAIL || userlist[index].uid == UNUSED )
		return (FAIL);
	userlist[index].usage += dinode[j].d_eblock - dinode[j].d_sblock + 1;
	return (SUCCEED);
}


output()
{
	int index;

	for (index=0; index < MAXUSERS ; index++)
		if ( userlist[index].uid != UNUSED && userlist[index].usage != 0 )
			printf("%ld	%s	%ld\n",
			    userlist[index].uid,
			    userlist[index].name,
			    userlist[index].usage);
}


unsigned
hash(j)
uid_t j;
{
	register unsigned start;
	register unsigned circle;
	circle = start = (unsigned)j % MAXUSERS;
	do
	{
		if ( userlist[circle].uid == j || userlist[circle].uid == UNUSED )
			return (circle);
		circle = (circle + 1) % MAXUSERS;
	} while ( circle != start);
	return (FAIL);
}

hashinit()
{
	int index;

	for(index=0; index < MAXUSERS ; index++)
	{
		userlist[index].uid = UNUSED;
		userlist[index].usage = 0;
		userlist[index].name[0] = '\0';
	}
}


static FILE *pwf = NULL;

setup(pfile)
char	*pfile;
{
	register struct passwd	*pw;
	void end_pwent();
	struct passwd *	(*getpw)();
	void	(*endpw)();
	int	index;

	if (pfile) {
		if( !stpwent(pfile)) {
			fprintf(stderr, "%s: Cannot open %s\n", cmd, pfile);
			exit(5);
		}
		getpw = fgetpwent;
		endpw = end_pwent;
	} else {
		setpwent();
		getpw = getpwent;
		endpw = endpwent;
	}
	while ( (pw=getpw(pwf)) != NULL )
	{
		if ( (index=hash(pw->pw_uid)) == FAIL )
		{
			fprintf(stderr,"%s: INCREASE SIZE OF MAXUSERS\n", cmd);
			return (FAIL);
		}
		if ( userlist[index].uid == UNUSED )
		{
			userlist[index].uid = pw->pw_uid;
			strncpy( userlist[index].name, pw->pw_name, MAXNAME);
		}
	}

	endpw();
}


char	*
skip(str)
register char	*str;
{
	while(*str) {
		if(*str == ' ' ||
		    *str == ',') {
			*str = '\0';
			str++;
			break;
		}
		str++;
	}
	return(str);
}


stpwent(pfile)
register char *pfile;
{
	if(pwf == NULL)
		pwf = fopen(pfile, "r");
	else
		rewind(pwf);
	return(pwf != NULL);
}

void
end_pwent()
{
	if(pwf != NULL) {
		(void) fclose(pwf);
		pwf = NULL;
	}
}

