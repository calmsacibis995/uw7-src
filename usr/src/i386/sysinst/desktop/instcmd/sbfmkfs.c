#ident	"%W	%T"

#include <stdio.h>
#include <locale.h>
#include <ctype.h>
#include <pfmt.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/mkdev.h>
#include <fs/mkfs.h>	/* exit code definitions */
#include <fs/memfs/memfs_mkroot.h>

#define UMASK		0755
#define	NAME_MAX	64
#define	FSTYPE		"memfs"

char	*myname, fstype[]=FSTYPE;

FILE 	*fin;
int	fsi;
int	fso_meta;
int	fso_data;
int 	fd;
char	*charp;

char	string[512];

char	*fdata;
char	*fmeta;
char	*fproto;
int	error = RET_OK;
int	meta_no;
int blocksize;		/* logical block size */
int verbose = 0;		/* verbose mode */

extern char *optarg;		/* getopt(3c) specific */
extern int optind;

void cfile(memfs_image_t *, int);
long	getnum();
void getstr();
void meta_open(char *);
void meta_close();
void new_meta(memfs_image_t *);
void newblk(ulong *);
void wtmeta(memfs_image_t *);
void wtfs(char *);
void usage();


main(argc, argv, envp)
int argc;
char *argv[];
char *envp[];
{
	int bflag = 0;
	int arg;
	memfs_image_t m;
	char label[NAME_MAX];
	time_t	cur_time;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxmkfs");
	myname = (char*)strrchr(argv[0],'/');
	myname = (myname != 0)? myname+1: argv[0];
	sprintf(label, "UX:%s %s", fstype, myname);
	(void)setlabel(label);

	time(&cur_time);

	/* Process the Options */
	while ((arg = getopt(argc,argv,"?b:v")) != -1) {
		switch(arg) {
		case 'b':
			bflag++;
			blocksize = atoi(optarg);
			break;
		case 'v':
			verbose++;
			break;
		case '?':	/* print usage message */
			usage();
		}
	}
	if (bflag) {
		if (blocksize  % 4096){
			pfmt(stderr, MM_ERROR,
				":18:%d is invalid logical block size\n", blocksize);
			exit(RET_BAD_BLKSZ);
		}
	} else
		blocksize = 4096;

	/* get the other arguments */
	fmeta = argv[optind++];
	fdata = argv[optind++];
	fproto = argv[optind++];

	fso_data = creat(fdata, 0666);
	if(fso_data < 0) {
		pfmt(stderr, MM_ERROR, ":22:cannot create %s\n", fdata);
		exit(RET_FSYS_CREATE);
	}

	meta_open(fmeta);

	if((fin = fopen(fproto, "r")) == 0){
		pfmt(stderr, MM_ERROR, "::cannot open %s\n",fproto);
		exit(RET_FSYS_OPEN);
	}

	strcpy( m.mi_name, "/");
	m.mi_atime.tv_sec = cur_time;
	m.mi_atime.tv_nsec = 0;
	m.mi_mtime.tv_sec = cur_time;
	m.mi_mtime.tv_nsec = 0;
	m.mi_ctime.tv_sec = cur_time;
	m.mi_ctime.tv_nsec = 0;
	cfile( &m,  0);

	/* write terminating memfs_image to mark end of array */
	new_meta(&m);
	wtmeta(&m);
	meta_close();

	exit(error);
}

void
cfile(m, parent)
memfs_image_t *m;
int	parent;
{
	memfs_image_t	*nm;
	ulong *db;
	int i, f, c;
	int	my_meta_no = meta_no;

	/*
	 * get mode, uid and gid
	 */

	getstr();

	m->mi_type  = gmode(string[0], "-bcdl", VREG, VBLK, VCHR, VDIR, VLNK, 0, 0);
	m->mi_mode = gmode(string[1], "-u", 0, S_ISUID, 0, 0, 0, 0, 0)
		| gmode(string[2], "-g", 0, S_ISGID, 0, 0, 0, 0, 0);
	
	for(i=3; i<6; i++) {
		c = string[i];
		if(c<'0' || c>'7') {
			pfmt(stderr, MM_ERROR,
				":39:%c/%s: bad octal mode digit\n", c, string);
			error = RET_BAD_OCTAL;
			c = 0;
		}
		m->mi_mode |= (c-'0')<<(15-3*i);
	}
	m->mi_uid = getnum();
	m->mi_gid = getnum();
	/*
	 * general initialization prior to
	 * switching on format
	 */

	m->mi_pnumber = parent;
	switch(m->mi_type) {

	case VLNK:
		/* symbolic link - path is the link it should point to */
		getstr();
		strcpy(m->mi_tname, string);
		break;

	case VREG:
		/*
		 * regular file
		 * contents is a file name
		 */
		if( (db = (ulong *)malloc((unsigned int)blocksize) ) == 0){
			printf("can't malloc %d blocksize\n", blocksize);
			exit(RET_MALLOC);
		}

		for(i=0; i<blocksize/sizeof(ulong); i++)
			db[i] = 0;

		getstr();
		f = open(string, 0);
		if(f < 0) {
			pfmt(stderr, MM_ERROR,
				":20:cannot open %s\n", string);
			error = RET_FILE_OPEN;
			break;
		}
		while((i=read(f, db, (unsigned int)blocksize)) > 0) {
			m->mi_size += i;
			newblk(db);
		}
		close(f);
		free(db);
		break;

	case VBLK:
	case VCHR:
		/*
		 * special file
		 * content is maj/min types
		 */

		i = getnum() & 0377;
		f = getnum() & 0377;
		m->mi_addr = makedev(i, f);
		break;

	case VDIR:
		/* special processing for a directory node.
		 * emit meta-data for directory then recurse for
		 * each child node.
		 */

		/*
		 * directory
		 * call recursively until name of "$" found
		 */

		wtmeta(m);
		if((nm = (memfs_image_t *)malloc(sizeof(memfs_image_t))) == 0){
			printf("cfile: can't malloc memfs_image\n");
			exit(RET_MALLOC);
		}
		while(1){
			new_meta(nm);
			getstr();
			if(string[0]=='$' && string[1]=='\0')
			{
				free(nm);
				return;
			}
			if((strlen(string) + strlen(m->mi_name)) > NAME_MAX-2 ){
				printf("WARN:: truncating pathname %s/%s\n",m->mi_name,string);
			}

			strcpy(nm->mi_name, string);
			cfile(nm, my_meta_no);
		}
		break;

	}
	wtmeta(m);
}

gmode(c, s, m0, m1, m2, m3, m4, m5, m6)
char c, *s;
{
	int i;

	for(i=0; s[i]; i++) {
		if(c == s[i]) {
			switch(i)
			{
			case 0: return(m0);
			case 1: return(m1);
			case 2: return(m2);
			case 3: return(m3);
			case 4: return(m4);
			case 5: return(m5);
			case 6: return(m6);
			}
		}
	}
	pfmt(stderr, MM_ERROR, ":41:%c/%s: bad mode\n", c, string);
	error = RET_BAD_MODE;
	return(0);
}

long 
getnum()
{
	int i, c;
	long n;

	getstr();
	n = 0;
	i = 0;
	while ((c=string[i]) > NULL){
		if(c<'0' || c>'9') {
			pfmt(stderr, MM_ERROR,
				":42:%s: bad number\n", string);
			error = RET_BAD_NUM;
			return((long)0);
		}
		n = n*10 + (c-'0');
		i++;
	}
	return(n);
}

void
getstr()
{
	int i, c;

	memset(string, '\0', 512);
loop:
	switch(c=getch()) {

	case ' ':
	case '\t':
	case '\n':
		goto loop;

	case '\0':
		pfmt(stderr, MM_ERROR, ":43:EOF\n");
		exit(RET_EOF);
		break;
	case EOF:
		pfmt(stderr, MM_ERROR, ":43:EOF\n");
		exit(RET_EOF);
		break;
	case ':':
		while(getch() != '\n');
		goto loop;
		break;
	}
	i = 0;

	do {
		string[i++] = c;
		c = getch();
	}while(c!=' '&&c!='\t'&&c!='\n'&&c!='\0');
	string[i] = '\0';
}

void
wtmeta(bf)
memfs_image_t *bf;
{
	char	*s;

	meta_no++;
	if( write(fso_meta, bf, sizeof(memfs_image_t)) != sizeof(memfs_image_t)) {
		pfmt(stderr, MM_ERROR,
			":45:meta write error: \n");
		exit(RET_ERR_WRITE);
	}

	switch(bf->mi_type) {

	case VNON:
		s = "VNON";
		break;
	case VLNK:
		s = "VLNK";
		break;
	case VREG:
		s = "VREG";
		break;
	case VBLK:
		s = "VBLK";
		break;
	case VCHR:
		s = "VCHR";
		break;
	case VDIR:
		s = "VDIR";
		break;
	default:
		s = "UNK";
		}

	if (verbose > 0){
		printf("\n");
		printf("meta: %d\n",meta_no-1);
		printf("mi_name: %s\n",bf->mi_name);
		printf("mi_size: %d mi_type: %s mi_mode: 0x%x 0%o mi_uid: %u mi_gid: %u\n",bf->mi_size,s,bf->mi_mode & S_IFMT,bf->mi_mode & MODEMASK,bf->mi_uid,bf->mi_gid);
		printf("mi_atime: 0x%x mi_mtime: 0x%x mi_ctime: 0x%x\n",
bf->mi_atime.tv_sec,bf->mi_mtime.tv_sec,bf->mi_ctime.tv_sec);
		printf("mi_pnumber: %d\n",bf->mi_pnumber);
		printf("mi_tname: %s\n",bf->mi_tname);
		if(bf->mi_type == VBLK || bf->mi_type == VCHR)
			printf("mi_addr: %x\n",bf->mi_addr);
	}
}

void
wtfs(bf)
char *bf;
{
	if( write(fso_data, bf, (unsigned int)blocksize) != blocksize) {
		pfmt(stderr, MM_ERROR,
			":45:data write error: \n");
		exit(RET_ERR_WRITE);
	}
}


void
newblk(db)
ulong *db;
{
	int i;

	wtfs((char *)db);
	for(i=0; i<blocksize/sizeof(ulong); i++)
		*(db+i) = 0;
}

getch()
{

	if(charp)
		return(*charp++);
	return(getc(fin));
}

void
usage()
{
	exit(RET_USAGE);
}

void
new_meta(m)
memfs_image_t *m;
{
	int i;

	m->mi_size = 0;
	m->mi_type = VNON;
	m->mi_mode = 0;
	m->mi_uid = 0;
	m->mi_gid = 0;
	m->mi_atime.tv_sec = 0;
	m->mi_atime.tv_nsec = 0;
	m->mi_mtime.tv_sec = 0;
	m->mi_mtime.tv_nsec = 0;
	m->mi_ctime.tv_sec = 0;
	m->mi_ctime.tv_nsec = 0;
	for (i=0; i<sizeof(m->mi_name);i++)
		m->mi_name[i] = '\0';
	for (i=0; i<sizeof(m->mi_tname);i++)
		m->mi_tname[i] = '\0';
}

void
meta_open( fmeta )
char *fmeta;
{
	fso_meta = creat(fmeta, 0666);
	if(fso_meta< 0) {
		pfmt(stderr, MM_ERROR, ":22:cannot create %s\n", fmeta);
		exit(RET_FSYS_CREATE);
	}
	meta_no = 0;
}

void
meta_close( )
{
	int	z;
	char	*m;

	if ( (z = blocksize-(meta_no * sizeof(memfs_image_t))%blocksize) != 0 ){
		m = (char *) calloc((unsigned int)z,sizeof(char));
		if( write(fso_meta, m, (unsigned int)z) != z) {
			pfmt(stderr, MM_ERROR,
				":45:meta write error: \n");
			exit(RET_ERR_WRITE);
		}
	}
}
