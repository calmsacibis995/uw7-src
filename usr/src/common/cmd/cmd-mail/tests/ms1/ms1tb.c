/*
 * our message store test program
 * knows both the ms1 cache API and the c-client API
 * this program is documented in the test suite document
 */
#ifdef OpenServer
# define MAXPATHLEN	PATHSIZE
# define MAXSYMLINKS	MAXLINKS
# define S_IAMB		0x1ff
#endif

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <pwd.h>
#include <utime.h>
#include <time.h>
#include "c-client/mail.h"
#include "c-client/scomsc1.h"

#define  uchar unsigned char
#define debug printf

void usage();
int p_access(char *);
int p_flags(char *);
void o_flags(FILE *, int);
void rb1();

void u_iunpack(char *, char *, char *);
void u_isetdel(char *);
void u_mchangelast(char *);
void u_mchangeindex(char *, int, int);
void u_msetutime(char *, char *);
void u_append(char *, char *);
void u_filldisk(char *);
void u_permcheck(int, char *, char *);
void u_lockfolder(char *, int, char *);
void u_init(int, int);
void u_bindata(char *, int);

int rebuild;

main(argc, argv)
char **argv;
{
	int yn;
	void *handle;
	int ret;
	char *cp;
	MESSAGECACHE elt;
	char datebuf[100];
	char date[50];
	char sender[512];
	char *rest;
	struct stat sbuf;
	int fd;
	scomsc1_stat_t *fs;
	int size;
	int perm;

#include "c-client/linkage.c"
	argc--;
	argv++;
	if (argc == 0)
		usage();
	scomsc1_set_rebuild_callback(rb1);
	while (argc > 0) {
		if (strcmp(*argv, "sendmail_parse_from") == 0) {
			if (argc < 3)
				usage();
			strcpy(datebuf, argv[2]);
			strcat(datebuf, "\nrest\n");
			date[0] = 0;
			sender[0] = 0;
			rest = 0;
			ret = sendmail_parse_from(datebuf, sender, date, &rest);
			ret = ret ? 1 : 0;
			yn = (*argv[1] == 'y');
			if (ret != yn)
				errout("sendmail_parse_from(%s)\n", argv[2]);
			argc -= 3; argv += 3;
			printf("date=%s\n", date);
			printf("sender=%s\n", sender);
			printf("rest=%s\n", rest ? rest : "(null)");
			continue;
		}
		/* message store cache API routines */
		if (strcmp(*argv, "scomsc1_valid") == 0) {
			if (argc < 3)
				usage();
			ret = scomsc1_valid(argv[2]);
			ret = ret ? 1 : 0;
			yn = (*argv[1] == 'y');
			if (ret != yn)
				errout("scomsc1_valid(%s)\n", argv[2]);
			argc -= 3; argv += 3;
			continue;
		}
		if (strcmp(*argv, "scomsc1_open") == 0) {
			if (argc < 6)
				usage();
			yn = (*argv[3] == 'y');
			rebuild = 0;
			handle = scomsc1_open(argv[4], p_access(argv[5]), yn);
			yn = (*argv[1] == 'y');
			if ((unsigned)handle < 3)
				ret = 0;
			else
				ret = 1;
			if (ret != yn)
				errout("scomsc1_open(%s, %s)\n", argv[4], argv[5]);
			/* check rebuild state */
			if (ret) {
				yn = (*argv[2] == 'y');
				if (yn != rebuild)
					errout("scomsc1_open(%s, %s) rebuild\n",
						argv[4], argv[5]);
			}
			if (ret == 0)
				handle = 0;
			argc -= 6; argv += 6;
			continue;
		}
		if (strcmp(*argv, "scomsc1_chopen") == 0) {
			if (argc < 3)
				usage();
			ret = scomsc1_chopen(handle, p_access(argv[2]));
			yn = (*argv[1] == 'y');
			if (ret != yn)
				errout("scomsc1_chopen(%x, %s)\n", handle, argv[2]);
			argc -= 3; argv += 3;
			continue;
		}
		if (strcmp(*argv, "scomsc1_close") == 0) {
			if (argc < 1)
				usage();
			if (handle)
				scomsc1_close(handle);
			handle = 0;
			argc -= 1; argv += 1;
			continue;
		}
		if (strcmp(*argv, "scomsc1_fetch") == 0) {
			if (argc < 4)
				usage();
			size = scomsc1_fetchsize(handle, atoi(argv[2]), atoi(argv[3]));
			cp = scomsc1_fetch(handle, 0, atoi(argv[2]), atoi(argv[3]));
			ret = cp ? 1 : 0;
			if (cp) {
				fwrite(cp, 1, size, stdout);
				fflush(stdout);
				free(cp);
			}
			yn = (*argv[1] == 'y');
			if (ret != yn)
				errout("scomsc1_fetch(%x, 0, %s, %s)\n",
					handle, argv[2], argv[3]);
			argc -= 4; argv += 4;
			continue;
		}
		if (strcmp(*argv, "scomsc1_fetchsize") == 0) {
			if (argc < 4)
				usage();
			ret = scomsc1_fetchsize(handle, atoi(argv[2]), atoi(argv[3]));
			printf("%d\n", ret); fflush(stdout);
			ret = (ret < 0) ? 0 : 1;
			yn = (*argv[1] == 'y');
			if (ret != yn)
				errout("scomsc1_fetchsize(%x, %s, %s)\n",
					handle, argv[2], argv[3]);
			argc -= 4; argv += 4;
			continue;
		}
		if (strcmp(*argv, "scomsc1_fetchlines") == 0) {
			if (argc < 4)
				usage();
			ret = scomsc1_fetchlines(handle, atoi(argv[2]), atoi(argv[3]));
			printf("%d\n", ret); fflush(stdout);
			ret = (ret < 0) ? 0 : 1;
			yn = (*argv[1] == 'y');
			if (ret != yn)
				errout("scomsc1_fetchlines(%x, %s, %s)\n",
					handle, argv[2], argv[3]);
			argc -= 4; argv += 4;
			continue;
		}
		if (strcmp(*argv, "scomsc1_fetchsender") == 0) {
			if (argc < 3)
				usage();
			ret = scomsc1_fetchsender(handle, atoi(argv[2]), datebuf);
			yn = (*argv[1] == 'y');
			if (ret != yn)
				errout("scomsc1_fetchsender(%x, %s, %s)\n",
					handle, argv[2], datebuf);
			printf("%s\n", datebuf); fflush(stdout);
			argc -= 3; argv += 3;
			continue;
		}
		if (strcmp(*argv, "scomsc1_fetchdate") == 0) {
			if (argc < 3)
				usage();
			ret = scomsc1_fetchdate(handle, atoi(argv[2]), &elt);
			yn = (*argv[1] == 'y');
			if (ret != yn)
				errout("scomsc1_fetchdate(%x, %s, %x)\n",
					handle, argv[2], &elt);
			mail_cdate(datebuf, &elt);
			datebuf[strlen(datebuf)-1] = 0;
			printf("%s %c%02d%02d\n", datebuf,
				elt.zoccident ? '-' : '+',
				elt.zhours, elt.zminutes);
			fflush(stdout);
			argc -= 3; argv += 3;
			continue;
		}
		if (strcmp(*argv, "scomsc1_fetchuid") == 0) {
			if (argc < 3)
				usage();
			ret = scomsc1_fetchuid(handle, atoi(argv[2]));
			printf("%d\n", ret); fflush(stdout);
			ret = ret ? 1 : 0;
			yn = (*argv[1] == 'y');
			if (ret != yn)
				errout("scomsc1_fetchuid(%x, %s)\n",
					handle, argv[2]);
			argc -= 3; argv += 3;
			continue;
		}
		if (strcmp(*argv, "scomsc1_setflags") == 0) {
			if (argc < 4)
				usage();
			ret = scomsc1_setflags(handle, atoi(argv[2]), p_flags(argv[3]));
			yn = (*argv[1] == 'y');
			if (ret != yn)
				errout("scomsc1_setflags(%x, %s, %s)\n",
					handle, argv[2], argv[3]);
			argc -= 4; argv += 4;
			continue;
		}
		if (strcmp(*argv, "scomsc1_getflags") == 0) {
			if (argc < 3)
				usage();
			ret = scomsc1_getflags(handle, atoi(argv[2]));
			if (ret >= 0)
				o_flags(stdout, ret);
			ret = (ret < 0) ? 0 : 1;
			yn = (*argv[1] == 'y');
			if (ret != yn)
				errout("scomsc1_getflags(%x, %s)\n",
					handle, argv[2]);
			argc -= 3; argv += 3;
			continue;
		}
		if (strcmp(*argv, "scomsc1_expunge") == 0) {
			if (argc < 2)
				usage();
			ret = scomsc1_expunge(handle);
			yn = (*argv[1] == 'y');
			if (ret != yn)
				errout("scomsc1_expunge(%x)\n", handle);
			argc -= 2; argv += 2;
			continue;
		}
		if (strcmp(*argv, "scomsc1_deliver") == 0) {
			if (argc < 6)
				usage();
			fd = open(argv[4], 0);
			if (fd < 0)
				errout("deliver: unable to open %s\n", argv[4]);
			fstat(fd, &sbuf); 
			size = sbuf.st_size;
			cp = (char *)malloc(size+1);
			if (cp == 0)
				errout("deliver: malloc out of memory\n");
			read(fd, cp, size);
			cp[size] = 0;
			if (sendmail_parse_from(cp, sender, date, &rest) == 0)
				errout("deliver: bad message envelope\n");
			rebuild = 0;
			ret = scomsc1_deliver(argv[3], sender, date,
				cp, size, p_flags(argv[5]));
			ret = (ret == 1) ? 1 : 0;
			yn = (*argv[1] == 'y');
			if (ret != yn)
				errout("scomsc1_deliver(%s, %s, %s)\n",
					argv[3], argv[4], argv[5]);
			/* check rebuild state */
			if (ret) {
				yn = (*argv[2] == 'y');
				if (yn != rebuild)
					errout("scomsc1_deliver(%s, %s, %s) rebuild\n",
						argv[3], argv[4], argv[5]);
			}
			free(cp);
			close(fd);
			argc -= 6; argv += 6;
			continue;
		}
		if (strcmp(*argv, "scomsc1_check") == 0) {
			if (argc < 2)
				usage();
			fs = scomsc1_check(handle);
			yn = (*argv[1] == 'y');
			ret = fs ? 1 : 0;
			if (ret != yn)
				errout("scomsc1_check(%x)\n", handle);
			printf("MSGS=%d\nVALIDITY=%x\n", fs->m_msgs,
				fs->m_validity);
			fflush(stdout);
			argc -= 2; argv += 2;
			continue;
		}
		if (strcmp(*argv, "scomsc1_create") == 0) {
			if (argc < 3)
				usage();
			rebuild = 0;
			ret = scomsc1_create(argv[2]);
			yn = (*argv[1] == 'y');
			if (ret != yn)
				errout("scomsc1_create(%s)\n", argv[2]);
			cp = argv[2];
			cp += strlen(cp);
			cp--;
			if (ret && (rebuild == 0) && (*cp != '/'))
				errout("scomsc1_create(%s) rebuild\n", argv[2]);
			argc -= 3; argv += 3;
			continue;
		}
		if (strcmp(*argv, "scomsc1_rename") == 0) {
			if (argc < 4)
				usage();
			ret = scomsc1_rename(argv[2], argv[3]);
			yn = (*argv[1] == 'y');
			if (ret != yn)
				errout("scomsc1_rename(%s, %s)\n",
					argv[2], argv[3]);
			argc -= 4; argv += 4;
			continue;
		}
		if (strcmp(*argv, "scomsc1_remove") == 0) {
			if (argc < 3)
				usage();
			ret = scomsc1_remove(argv[2]);
			yn = (*argv[1] == 'y');
			if (ret != yn)
				errout("scomsc1_remove(%s)\n",
					argv[2]);
			argc -= 3; argv += 3;
			continue;
		}
		/* c-client API routines */
		/* utility routines */
		if (strcmp(*argv, "u_iunpack") == 0) {
			if (argc < 4)
				usage();
			u_iunpack(argv[1], argv[2], argv[3]);
			argc -= 4; argv += 4;
			continue;
		}
		if (strcmp(*argv, "u_isetdel") == 0) {
			if (argc < 2)
				usage();
			u_isetdel(argv[1]);
			argc -= 2; argv += 2;
			continue;
		}
		if (strcmp(*argv, "u_mchangelast") == 0) {
			if (argc < 2)
				usage();
			u_mchangelast(argv[1]);
			argc -= 2; argv += 2;
			continue;
		}
		if (strcmp(*argv, "u_mchangeindex") == 0) {
			if (argc < 4)
				usage();
			u_mchangeindex(argv[1], atoi(argv[2]), *argv[3]);
			argc -= 4; argv += 4;
			continue;
		}
		if (strcmp(*argv, "u_msetutime") == 0) {
			if (argc < 3)
				usage();
			u_msetutime(argv[1], argv[2]);
			argc -= 3; argv += 3;
			continue;
		}
		if (strcmp(*argv, "u_append") == 0) {
			if (argc < 3)
				usage();
			u_append(argv[1], argv[2]);
			argc -= 3; argv += 3;
			continue;
		}
		if (strcmp(*argv, "u_sleep") == 0) {
			if (argc < 2)
				usage();
			sleep(atoi(argv[1]));
			argc -= 2; argv += 2;
			continue;
		}
		if (strcmp(*argv, "u_filldisk") == 0) {
			if (argc < 2)
				usage();
			u_filldisk(argv[1]);
			argc -= 2; argv += 2;
			continue;
		}
		if (strcmp(*argv, "u_permcheck") == 0) {
			if (argc < 4)
				usage();
			perm = strtol(argv[1], 0, 8);
			u_permcheck(perm, argv[2], argv[3]);
			argc -= 4; argv += 4;
			continue;
		}
		if (strcmp(*argv, "u_lockfolder") == 0) {
			if (argc < 4)
				usage();
			u_lockfolder(argv[1], p_access(argv[2]), argv[3]);
			argc -= 4; argv += 4;
			continue;
		}
		if (strcmp(*argv, "u_init") == 0) {
			if (argc < 3)
				usage();
			u_init(atoi(argv[1]), atoi(argv[2]));
			argc -= 3; argv += 3;
			continue;
		}
		if (strcmp(*argv, "u_rm") == 0) {
			if (argc < 2)
				usage();
			unlink(argv[1]);
			argc -= 2; argv += 2;
			continue;
		}
		if (strcmp(*argv, "u_timezone") == 0) {
			if (argc < 1)
				usage();
			tzset();
			ret = timezone/60;
			printf("-%02d%02d\n", ret/60, ret%60);
			argc -= 1; argv += 1;
			continue;
		}
		if (strcmp(*argv, "u_bindata") == 0) {
			if (argc < 3)
				usage();
			ret = (*argv[2] == 'y') ? 1 : 0;
			u_bindata(argv[1], ret);
			argc -= 3; argv += 3;
			continue;
		}
		printf("Unrecognized command: %s\n", argv[0]);
		exit(1);
	}
	exit(0);
}

/*
 * parse access types, separated by | matching the ACCESS defines
 */
int
p_access(char *str)
{
	char *cp;
	char *cp1;
	int access;

	cp = str;
	access = 0;
	while (*cp) {
		cp1 = strchr(cp, '|');
		if (cp1)
			*cp1 = 0;
		if (strcmp(cp, "ACCESS_SE") == 0)
			access |= ACCESS_SE;
		else if (strcmp(cp, "ACCESS_AP") == 0)
			access |= ACCESS_AP;
		else if (strcmp(cp, "ACCESS_RDX") == 0)
			access |= ACCESS_RDX;
		else if (strcmp(cp, "ACCESS_RD") == 0)
			access |= ACCESS_RD;
		else if (strcmp(cp, "ACCESS_BOTH") == 0)
			access |= ACCESS_BOTH;
		else if (strcmp(cp, "ACCESS_FOLDER") == 0)
			access |= ACCESS_FOLDER;
		else if (strcmp(cp, "ACCESS_INDEX") == 0)
			access |= ACCESS_INDEX;
		else if (strcmp(cp, "ACCESS_REBUILD") == 0)
			access |= ACCESS_REBUILD;
		else
			errout("Bad access string: %s\n", cp);
		if (cp1) {
			*cp1 = '|';
			cp = cp1 + 1;
		}
		else
			cp += strlen(cp);
	}
	return(access);
}

/*
 * parse flags a single string of upper case letters converted into flags
 */
int
p_flags(char *flagstr)
{
	int flags;
	char *cp;

	cp = flagstr;
	flags = 0;
	while (*cp) {
		switch (*cp) {
		case 'R':
			flags |= fSEEN;
			break;
		case 'O':
			flags |= fOLD;
			break;
		case 'D':
			flags |= fDELETED;
			break;
		case 'F':
			flags |= fFLAGGED;
			break;
		case 'A':
			flags |= fANSWERED;
			break;
		case 'T':
			flags |= fDRAFT;
			break;
		default:
			errout("bad flag: %c from: %s\n", *cp, flagstr);
			exit(1);
		}
		cp++;
	}

	return(flags);
}

/*
 * output flags
 */
void
o_flags(FILE *ofd, int flags)
{
	fprintf(ofd, "status=");
	if (flags&fSEEN) fprintf(ofd, "R");
	if (flags&fOLD) fprintf(ofd, "O");
	if (flags&fDELETED) fprintf(ofd, "D");
	if (flags&fFLAGGED) fprintf(ofd, "F");
	if (flags&fANSWERED) fprintf(ofd, "A");
	if (flags&fDRAFT) fprintf(ofd, "T");
	fprintf(ofd, "\n");
}

/*
 * convert index binary file into asci hex for comparison and storage purposes
 * check folder mode time and index file size
 */
void
u_iunpack(char *dst, char *src, char *mbox)
{
	FILE *ifd;
	FILE *ofd;
	mrd_t mrd;
	mrc_t mrc;
	prd_t prd;
	prc_t prc;
	int i;
	struct stat sbuf;

	ifd = fopen(src, "r");
	if (ifd == 0)
		errout("u_iunpack: Unable to open %s\n", src);
	ofd = fopen(dst, "w");
	if (ofd == 0) {
		fclose(ifd);
		errout("u_iunpack: Unable to create %s\n", dst);
	}
	if (fread(&mrd, 1, sizeof(mrd_t), ifd) != sizeof(mrd_t)) {
		fclose(ifd);
		fclose(ofd);
		errout("u_iunpack: Unable to read master record\n");
	}
	/* have master record */
	msc1_mrc_in(&mrc, &mrd);

	/* first check that mod time is valid */
	stat(mbox, &sbuf);
	if (sbuf.st_mtime != mrc.mrc_mtime)
		errout("u_iunpack(%s, %s, %s)\n    mod time not match: folder %08x index %08x\n", dst, src, mbox, sbuf.st_mtime, mrc.mrc_mtime);

	if (mrc.mrc_mtime < mrc.mrc_validity)
		errout("u_iunpack(%s, %s, %s)\n    bad validity: folder %08x index %08x\n", dst, src, mbox, sbuf.st_mtime, mrc.mrc_mtime);
	if (mrc.mrc_fsize != sbuf.st_size)
		errout("u_iunpack(%s, %s, %s)\n    folder size not match\n", dst, src, mbox);

	/* check index file size is correct */
	stat(src, &sbuf);
	i = sizeof(mrd_t) + sizeof(prd_t)*mrc.mrc_msgs;
	if (sbuf.st_size != i)
		errout("u_iunpack index size not match\n    expected file size %d actual file size %d\n", i, sbuf.st_size);
	/* output master record */
	fprintf(ofd, "mrc_magic=\"%s\"\n", mrd.mrd_magic);
	fprintf(ofd, "mrc_fsize=%d\n", mrc.mrc_fsize);
	fprintf(ofd, "mrc_validity=%08x\n", mrc.mrc_validity);
	fprintf(ofd, "mrc_uid_next=%d\n", mrc.mrc_uid_next);
	fprintf(ofd, "mrc_msgs=%d\n", mrc.mrc_msgs);
	fprintf(ofd, "mrc_mmdf=%d\n", mrc.mrc_mmdf);
	fprintf(ofd, "mrc_status=0x%x\n", mrc.mrc_status);
	/*
	 * this breaks shell if quotes are in mail message bodies
	 * we don't use quotes in our test messages
	 */
	fprintf(ofd, "mrc_consistency=\"%s\"\n", mrc.mrc_consistency);
	for (i = 0; i < mrc.mrc_msgs; i++) {
		if (fread(&prd, 1, sizeof(prd_t), ifd) != sizeof(prd_t)) {
			fclose(ifd);
			fclose(ofd);
			errout("u_iunpack: Unable to read message record %d\n", i);
		}
		/* output message record */
		msc1_prc_in(&prc, &prd);
		fprintf(ofd, "prc%d_start=%d\n", i, prc.prc_start);
		fprintf(ofd, "prc%d_size=%d\n", i, prc.prc_size);
		fprintf(ofd, "prc%d_hdrstart=%d\n", i, prc.prc_hdrstart);
		fprintf(ofd, "prc%d_hdrlines=%d\n", i, prc.prc_hdrlines);
		fprintf(ofd, "prc%d_bodystart=%d\n", i, prc.prc_bodystart);
		fprintf(ofd, "prc%d_bodylines=%d\n", i, prc.prc_bodylines);
		fprintf(ofd, "prc%d_uid=%d\n", i, prc.prc_uid);
		fprintf(ofd, "prc%d_stat_start=%d\n", i, prc.prc_stat_start);
		fprintf(ofd, "prc%d_", i);
		o_flags(ofd, prc.prc_status);
		fprintf(ofd, "prc%d_date=\"%02d/%02d/%02d %02d:%02d:%02d %c%02d%02d\"\n", i,
			prc.prc_month, prc.prc_day, prc.prc_year + 1970,
			prc.prc_hours, prc.prc_minutes, prc.prc_seconds,
			prc.prc_zoccident ? '-' : '+',
			prc.prc_zhours, prc.prc_zminutes);
	}
	fclose(ofd);
	fclose(ifd);
}

/*
 * set delete and update flag in index file, all other things are preserved
 */
void
u_isetdel(char *path)
{
	FILE *fp;
	mrd_t mrd;
	mrc_t mrc;

	fp = fopen(path, "r+");
	if (fp == 0) {
		fclose(fp);
		errout("u_isetdel: Unable to open %s\n", path);
	}
	if (fread(&mrd, 1, sizeof(mrd_t), fp) != sizeof(mrd_t)) {
		fclose(fp);
		errout("u_isetdel: Unable to read master record\n");
	}
	msc1_mrc_in(&mrc, &mrd);
	mrc.mrc_status |= MSC1_UPDATE;
	msc1_mrc_out(&mrd, &mrc);
	rewind(fp);
	if (fwrite(&mrd, 1, sizeof(mrd_t), fp) != sizeof(mrd_t)) {
		fclose(fp);
		errout("u_isetdel: Unable to write master record\n");
	}
	fclose(fp);
}

/*
 * change second to last char of mailbox to 'X', preserve mtime.
 */
void
u_mchangelast(char *path)
{
	FILE *fp;
	struct utimbuf tbuf;
	struct stat sbuf;

	fp = fopen(path, "r+");
	if (fp == 0) {
		fclose(fp);
		errout("u_mchangelast: Unable to open %s\n", path);
	}
	if (fstat(fileno(fp), &sbuf) < 0) {
		fclose(fp);
		errout("u_mchangelast: Unable to stat %s\n", path);
	}
	tbuf.actime = sbuf.st_mtime;
	tbuf.modtime = sbuf.st_mtime;
	if (fseek(fp, -2, SEEK_END) < 0) {
		fclose(fp);
		errout("u_mchangelast: Unable to fseek %s\n", path);
	}
	if (fwrite("X", 1, 1, fp) != 1) {
		fclose(fp);
		errout("u_mchangelast: Unable to write X character\n");
	}
	fclose(fp);
	if (utime(path, &tbuf) < 0)
		errout("u_mchangelast: unable to set utime\n");
}

/*
 * change arbitrary character in mailbox to character given
 * reset times to fool normal checking
 */
void
u_mchangeindex(char *path, int index, int val)
{
	FILE *fp;
	struct utimbuf tbuf;
	struct stat sbuf;
	char buf[2];

	fp = fopen(path, "r+");
	if (fp == 0) {
		fclose(fp);
		errout("u_mchangeindex: Unable to open %s\n", path);
	}
	if (fstat(fileno(fp), &sbuf) < 0) {
		fclose(fp);
		errout("u_mchangeindex: Unable to stat %s\n", path);
	}
	tbuf.actime = sbuf.st_mtime;
	tbuf.modtime = sbuf.st_mtime;
	if (fseek(fp, index, SEEK_SET) < 0) {
		fclose(fp);
		errout("u_mchangeindex: Unable to fseek %s\n", path);
	}
	buf[0] = val;
	if (fwrite(buf, 1, 1, fp) != 1) {
		fclose(fp);
		errout("u_mchangeindex: Unable to write X character\n");
	}
	fclose(fp);
	if (utime(path, &tbuf) < 0)
		errout("u_mchangeindex: unable to set utime\n");
}

/*
 * make mailbox mod time match index, for extended consistency checks
 */
void
u_msetutime(char *folder, char *index)
{
	FILE *fp;
	mrd_t mrd;
	mrc_t mrc;
	struct utimbuf tbuf;
	struct stat sbuf;

	fp = fopen(index, "r+");
	if (fp == 0) {
		fclose(fp);
		errout("u_msetutime: Unable to open %s\n", index);
	}
	if (fread(&mrd, 1, sizeof(mrd_t), fp) != sizeof(mrd_t)) {
		fclose(fp);
		errout("u_msetutime: Unable to read master record\n");
	}
	msc1_mrc_in(&mrc, &mrd);
	fclose(fp);

	tbuf.actime = mrc.mrc_mtime;
	tbuf.modtime = mrc.mrc_mtime;
	if (utime(folder, &tbuf) < 0)
		errout("u_msetutime: unable to set utime on %s\n", folder);
}

/*
 * do a simple append to a folder without index update.
 */
void
u_append(char *folder, char *message)
{
	FILE *fd1;
	FILE *fd2;
	char buf[BUFSIZ];
	int len;

	fd1 = fopen(folder, "a");
	if (fd1 == 0)
		errout("u_append: Unable to open %s\n", folder);
	fd2 = fopen(message, "r");
	if (fd2 == 0)
		errout("u_append: Unable to open %s\n", message);
	while (len = fread(buf, 1, BUFSIZ, fd2))
		if (fwrite(buf, 1, len, fd1) != len)
			errout("u_append: Write error on %s\n", message);
	fclose(fd1);
	fclose(fd2);
}

/*
 * fill up disk then back off 1k
 */
void
u_filldisk(char *path)
{
#define FILLDISK	1024
	int fd;
	char buf[FILLDISK];
	int size;

	fd = creat(path, 0666);
	if (fd < 0)
		errout("Unable to create %s\n", path);
	size = 0;
	while (write(fd, buf, FILLDISK) > 0)
		size += FILLDISK;
	size -= FILLDISK;
	ftruncate(fd, size);
	close(fd);
}

/*
 * verify permissions on folder and index (0600)
 */
void
u_permcheck(int perm, char *folder, char *index)
{
	struct stat sbuf;

	if (stat(folder, &sbuf))
		errout("permcheck: Unable to stat %s\n", folder);
	if ((sbuf.st_mode&S_IAMB) != perm)
		errout("permcheck: Permissions wrong on %s\n", folder);
	if (stat(index, &sbuf))
		errout("permcheck: Unable to stat %s\n", index);
	if ((sbuf.st_mode&S_IAMB) != perm)
		errout("permcheck: Permissions wrong on %s\n", index);
}

/*
 * open and lock a folder, sit in back ground, leave kill file around
 */
void
u_lockfolder(char *folder, int access, char *killfile)
{
	void *handle;
	int pid;
	FILE *fd;

	if ((pid = fork()) == 0) {
		/* child */
		handle = scomsc1_open(folder, access, 0);
		if ((handle == 0) || (handle == (void *)1) || (handle == (void *)2))
			errout("lockfolder child: could not open %s\n", folder);
		/* make sure we realy have it (BOTH gets lost in open),
		   as BOTH is not intended to be visible above the API */
		if (scomsc1_chopen(handle, access) == 0)
			errout("lockfolder child: could not lock %s\n", folder);
		sleep(30);
		exit(0);
	}
	/* race condition, safe on a quiescent machine */
	sleep(2);
	fd = fopen(killfile, "w");
	if (fd == 0) {
		kill(pid, 1);
		errout("lockfolder: Unable to create %s\n", killfile);
	}
	fprintf(fd, "kill %d\n", pid);
	fclose(fd);
}

/*
 * call msc1_init(restart)
 * print out config info in /etc/default/mail compatible syntax
 */
void
u_init(int restart, int output)
{
	int tmp;

	msc1_init(restart);
	if (output == 0)
		return;
	printf("MS1_FOLDER_FORMAT=%s\n", conf_mmdf ? "MMDF" : "Sendmail");
	printf("MS1_INBOX_DIR=%s\n", conf_inbox_dir ? conf_inbox_dir : "");
	printf("MS1_INBOX_NAME=%s\n", conf_inbox_name ? conf_inbox_name : "");
	printf("MS1_FSYNC=%s\n", conf_fsync ? "TRUE" : "FALSE");
	printf("MS1_EXTENDED_CHECKS=%s\n", conf_extended_checks ? "TRUE" : "FALSE");
	printf("MS1_EXPUNGE_THRESHOLD=%d\n", conf_expunge_threshold);
	printf("MS1_FOLDERS_INCORE=%s\n", conf_long_cache ? "TRUE" : "FALSE");
	printf("MS1_FILE_LOCKING=%s\n", conf_file_locking ? "TRUE" : "FALSE");
	printf("MS1_LOCK_TIMEOUT=%d\n", conf_lock_timeout);
	tmp = (~conf_umask)&0777;
	printf("MS1_UMASK=%#o\n", tmp);
}

/*
 * append binary data to file (all chars including null are checked)
 */
void
u_bindata(char *path, int newline)
{
	FILE *fd;
	int i;

	fd = fopen(path, "a");
	if (fd == 0)
		errout("u_bindata: Unable to open %s for append\n", path);
	for (i = 0; i < 256; i++)
		fputc(i, fd);
	if (newline)
		fputc('\n', fd);
	fclose(fd);
}

/* VARARGS */

errout(char *str, void *a, void *b, void *c, void *d, void *e)
{
	printf("Error: ");
	printf(str, a, b, c, d, e);
	exit(1);
}

void
rb1()
{
	rebuild = 1;
}

char *helptxt[] = {
"usage: ms1tb options...",
"",
"Cache API entries",
"  scomsc1_valid         yn path",
"  scomsc1_open          yn yn yn path access  - ACCESS_SE|AP|RD|RDX|FOLDER|INDEX|BOTH",
"  scomsc1_chopen        yn access",
"  scomsc1_close",
"  scomsc1_fetch         yn msgno body      - contents are written to stdout",
"  scomsc1_fetchsize     yn msgno body      - count is written to stdout",
"  scomsc1_fetchdate     yn msgno           - envelope date is written to stdout",
"  scomsc1_fetchuid      yn msgno           - uid is written to stdout",
"  scomsc1_fetchsender   yn msgno           - envelope sender is sent to stdout",
"  scomsc1_setflags      yn msgno flags",
"  scomsc1_getflags      yn msgno           - flags written in hex to stdout",
"  scomsc1_expunge       yn",
"  scomsc1_deliver       yn yn folder msg_file flags",
"  scomsc1_check         yn                 - MSGS= and VALIDITY= to stdout",
"  scomsc1_create        yn path",
"  scomsc1_rename        yn newpath oldpath",
"  scomsc1_remove        yn path",
"  sendmail_parse_from   yn file             - from line is parsed",
"C-Client API entries",
"Utility entries",
"  u_iunpack             dst src folder     - convert index file to ascii",
"                                             checks folder mod date as well",
"  u_isetdel             dst                - set DELETED and UPDATE flags",
"  u_mchangelast         dst                - second to last char X",
"  u_mchangeindex        dst index char     - char given is set in folder",
"  u_msetutime           folder index       - make utime on folder match index",
"  u_append              mailbox msgfile    - do simple append",
"  u_sleep               n                  - sleep n seconds",
"  u_filldisk            path               - fill disk, back off 1k"
"  u_permcheck           perm folder index  - check perms on folder and index",
"  u_lockfolder          folder access killfile",
"     background process to open and lock folder",
"     leaves shell script to kill itself in killfile",
"  u_init                newuser output     - calls msc1_init(newuser)",
"     non-zero output means dump config results to stdout",
"  u_rm                  path               - calls unlink(path)",
"  u_timezone                               - prints out current timezone\n",
"  u_bindata             path nl            - appends binary data to file\n",
"     nl is y for include newline at end, n for not",
"     output prints config values to stdout",
"Notes",
"  yn - expect failure or success, program returns failure if not match",
"  yn - second yn (on open and deliver) means expect rebuild or not",
"       failure is returned if not match",
"  yn - third yn (on open) is the convert flag passed to open",
"This program treats arguments as a script.",
"Multiple commands followed by their options can be given.",
"In particular the flags, fetches, and message delete calls must",
"be preceded by an open.",
0,
};

void
usage()
{
	char **cp;

	for (cp = helptxt; *cp; cp++)
		printf("%s\n", *cp);
	fflush(stdout);

	exit(1);
}
