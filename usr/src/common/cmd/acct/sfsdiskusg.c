/*		copyright	"%c%" 	*/

#ident	"@(#)acct:common/cmd/acct/sfsdiskusg.c	1.1.1.3"
#ident "$Header$"

/*
 * sfsdiskusg: diskusg for sfs
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <pwd.h>
#include <fcntl.h>
#include "acctdef.h"
#include <sys/vnode.h>
#include <sys/fs/sfs_inode.h>
#include <sys/stat.h>
#include <sys/fs/sfs_fs.h>

#define max(a,b)        ((a)>(b)?(a):(b))

struct	dinode	itab[MAXIPG];	/* MAXIPG is max inodes per cyl grp */
ino_t	ino;			/* used by ilist() and count() */
long	lseek();
int	VERBOSE = 0;
FILE	*ufd = 0;	/* fd for file where unacct'd for fileusg goes */
char	*ignlist[MAXIGN];	/* ignore list of filesystem names */
int	igncnt = {0};
char	*cmd;
unsigned hash();

union {
	struct	fs	sblk;
	char xxx[SBSIZE];	/* because fs is variable length */
} real_fs;
#define sblock real_fs.sblk

struct acct  {
	uid_t	uid;
	long	usage;
	char	name [MAXNAME+1];
} userlist [MAXUSERS];

main(argc, argv)
int argc;
char **argv;
{
	extern	int	optind;
	extern	char	*optarg;
	register c;
	register FILE	*fd;
	int 	rfd;
	struct	stat	sb;
	int	sflg = {FALSE};
	char 	*pfile = NULL;
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
		ignore(optarg);
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
			if (ufd) close(ufd);
		}
	}
	output();
	exit(0);
}

ilist(file, fd)
char *file;
register fd;
{
	register int j, c;

	if (fd  < 0) {
		fprintf(stderr, "%s: Cannot open %s\n", cmd, file); 
		return (FAIL);
	}
	sync();

	/* Read in superblock of the file system */
	bread(fd, SBLOCK, (char *)&sblock, SBSIZE);

	/* Check for file system names to ignore */
	if (!todo()) 
		return;

	if (sblock.fs_magic != SFS_MAGIC) { 
		fprintf(stderr, "%s: %s not a sfs file system, ignored\n", cmd, file);
		return (-1);
	}

	/*
	 * traverse the ilist by cylinder groups
	 */
	for (ino = 0, c = 0; c < sblock.fs_ncg; c++) {
		/*
		 * fsbtodb() translates a block number to dev addr
		 * cgimin() give fs addr of inode block
		 */
		bread(fd, fsbtodb(&sblock, cgimin(&sblock, c)), (char *)itab,
		   (int)(sblock.fs_ipg * sizeof (struct dinode)));
		/*
		 * sfs uses a pair of inodes for each file.  The first 
		 * inode of the pair (even) is like ufs inodes, the second
		 * inode of the pair (odd) is for secure info only.  We
		 * only count the even inodes.
		 */
		for(j = 0; j < sblock.fs_ipg; j+=2) {
			if (itab[j].di_smode != 0) {
				if(count(&itab[j]) == FAIL) {
					if(VERBOSE)
						fprintf(stderr,"BAD UID: file system = %s, inode = %u, uid = %ld\n",
					    	file, ino, itab[j].di_uid);
					if(ufd)
						fprintf(ufd, "%s %u %ld\n", file, ino, itab[j].di_uid);
				}
			}
			ino++;
		}
	}
	return (0);
}

count(ip)
struct dinode *ip;
{
	int index;

	if ( ip->di_nlink == 0 || ip->di_mode == 0 )
		return(SUCCEED);
	if( (index = hash(ip->di_uid)) == FAIL || 
		userlist[index].uid == UNUSED )
		return (FAIL);
/* for debugging
	printf("ino: %d, uid: %d, blocks %d\n", ino, ip->di_uid, ip->di_blocks);
 */
	userlist[index].usage += ip->di_blocks;
	return (SUCCEED);
}

bread(fd, bno, buf, cnt)
register fd;
daddr_t bno;
char *buf;
int cnt;
{
	register i;
	int got;

	if (lseek(fd, (long)(bno * DEV_BSIZE), 0) == (long) -1) {
		(void) fprintf(stderr, "%s: lseek error %d\n", 
				cmd, bno * DEV_BSIZE);
		for(i=0; i < cnt; i++)
			buf[i] = 0;
		return;
	}

	got = read((int)fd, buf, cnt);
	if (got != cnt) {
		perror ("read");
		(void) fprintf(stderr, 
			"%s: (wanted %d got %d blk %d)\n", cmd, cnt, got, bno);
		for(i=0; i < cnt; i++)
			buf[i] = 0;
	}
}

adduser(fd)
register FILE	*fd;
{
	uid_t	usrid;
	long	blcks;
	char	login[MAXNAME+10];
	int 	index;

	while(fscanf(fd, "%ld %s %ld\n", &usrid, login, &blcks) == 3) {
		if( (index = hash(usrid)) == FAIL) return(FAIL);
		if(userlist[index].uid == UNUSED) {
			userlist[index].uid = usrid;
			strncpy(userlist[index].name, login, MAXNAME);
		}
		userlist[index].usage += blcks;
	}
}

ignore(str)
register char	*str;
{
	char	*skip();

	for( ; *str && igncnt < MAXIGN; str = skip(str), igncnt++)
		ignlist[igncnt] = str;
	if(igncnt == MAXIGN) {
		fprintf(stderr, "%s: ignore list overflow. Recompile with larger MAXIGN\n", cmd);
	}
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

	for(index=0; index < MAXUSERS ; index++) {
		userlist[index].uid = UNUSED;
		userlist[index].usage = 0;
		userlist[index].name[0] = '\0';
	}
}

todo()
{
	int	i, len;
	char	fsname[6];

	what_is_your_name(fsname);
	for(i = 0; i < igncnt; i++) {
		len = max(strlen(fsname), strlen(ignlist[i])); 
		if(strncmp(fsname, ignlist[i], len) == 0) 
			return(FALSE);
	}
	return(TRUE);
}

/*
 * find the superblock's s_fname, easy in s5, kinda painful here.
 */ 

what_is_your_name(fsname)
char *	fsname;
{
	int	blk;
	int	i;
	char	*p;

	/*
	 * Code below taken from label() in
	 * /usr/src/cmd/fs.d/sfs/labelit/labelit.c
	 */
	blk = sblock.fs_spc * sblock.fs_cpc / NSPF(&sblock);
	for (i = 0; i < blk; i += sblock.fs_frag)
		/* void */;
	i -= sblock.fs_frag;
	blk = i / sblock.fs_frag;
	p = (char *)&(sblock.fs_rotbl[blk]);
	for (i = 0; (i < 6) && (*p); i++, p++)
		fsname[i] = *p;
}

static FILE *pwf = NULL;

setup(pfile)
char	*pfile;
{
	register struct passwd	*pw;
	void end_pwent();
	struct passwd *	(*getpw)();
	void	(*endpw)();
	int index;

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
	while ( (pw=getpw(pwf)) != NULL ) {
		if ( (index=hash(pw->pw_uid)) == FAIL ) {
			fprintf(stderr,"%s: INCREASE SIZE OF MAXUSERS\n", cmd);
			return (FAIL);
		}
		if ( userlist[index].uid == UNUSED ) {
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

