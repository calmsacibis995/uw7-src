#ident	"@(#)dstime:dstime/dst_next.c	1.2"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tzfile.h>

#define	SYSTZ_FILE	"/etc/TIMEZONE"

struct zone {
	long		offset;	/* offset from UTC (negative of file's) */
	unsigned char	isalt;	/* is an alternate timezone */
	unsigned char	index;	/* timezone name strs index */
};

struct tz_file {
	char		*strs;	/* timezone names */
	unsigned char	*kinds;	/* indices for dates[] into zones[] */
	long		*dates;	/* when timezone changes occur (UTC) */
	struct zone	*zones;	/* distinct timezones */
	int		count;	/* length of kinds[] and dates[] */
	int		npath;	/* string length of path[] */
	short		reg;	/* main timezone index (if >= 0) */
	short		alt;	/* alternate timezone index (if >= 0) */
	char		path[1];
};

struct tz_info /* information from TZ environment variable */
{
	char	*etc;		/* nonzero => further information to go */
	long	off[2];		/* main, alt timezone offsets from UTC */
	char	str[2][TZNAME_MAX + 1];	/* name for main, alt timezones */
};

static long
decode(const char *s)
{
	register const unsigned char *p = (const unsigned char *)s;

#if CHAR_BIT == 8
	return ((long)p[0] << 24) | ((long)p[1] << 16) | (p[2] << 8) | p[3];
#else
	return ((long)(p[0] & 0xff) << 24)
		| ((long)(p[1] & 0xff) << 16)
		| ((p[2] & 0xff) << 8) | (p[3] & 0xff);
#endif
}

static struct tz_file *
update(const char *file)
{
	static const char tzdef[] = TZDEFAULT;
	static const char tzdir[] = TZDIR;
	static struct tz_file *zp;
	struct tzhead header;
	long ntime, ntype, nchar;
	FILE *fp;
	size_t n;

	if (*file == '\0')
		file = tzdef;
	if (zp != 0) {
		if (*file == '/') {
			if (strcmp(file, zp->path) == 0)
				return zp;
		} else if (zp->npath > sizeof(tzdir)) {
			if (strcmp(file, &zp->path[sizeof(tzdir)]) == 0)
				return zp;
		}
		/*
		 * Don't expect to reach this code too often.
		 */
		free((void *)zp->zones);	/* includes strs[] */
		free((void *)zp->dates);	/* includes kinds[] */
		free((void *)zp);		/* includes path[] */
	}
	n = strlen(file);
	if (*file == '/') {
		if ((zp = (struct tz_file *)malloc(sizeof(struct tz_file)
			+ n)) == 0) {
			return 0;
		}
		(void)strcpy(zp->path, file);
		zp->npath = n;
	} else {
		if ((zp = (struct tz_file *)malloc(sizeof(struct tz_file)
			+ sizeof(tzdir) + n)) == 0) {
			return 0;
		}
		(void)strlist(zp->path, tzdir, "/", file, (char *)0);
		zp->npath = sizeof(tzdir) + n;
		file = zp->path;
	}
	if ((fp = fopen(file, "r")) == 0) {
	err0:;
		free((void *)zp);
		zp = 0;
		return 0;
	}
	if (fread((void *)&header, sizeof(header), 1, fp) != 1
		|| (ntime = decode(header.tzh_timecnt)) < 0
		|| ntime > TZ_MAX_TIMES
		|| (ntype = decode(header.tzh_typecnt)) <= 0
		|| ntype > TZ_MAX_TYPES
		|| (nchar = decode(header.tzh_charcnt)) <= 0
		|| nchar > TZ_MAX_CHARS) {
	err1:;
		fclose(fp);
		goto err0;
	}
	if ((zp->count = ntime) != 0) {
		if ((zp->dates = (long *)malloc(ntime * sizeof(long)
			+ ntime)) == 0) {
			goto err1;
		}
		zp->kinds = (unsigned char *)&zp->dates[ntime];
	} else if (ntype != 1) /* if no times given, only one zone can exist */
		goto err1;
	if ((zp->zones = (struct zone *)malloc(ntype * sizeof(struct zone)
		+ nchar + 1)) == 0) {
	err2:;
		if (zp->count != 0)
			free((void *)zp->dates);
		goto err1;
	}
	zp->strs = (char *)&zp->zones[ntype];
	zp->strs[nchar] = '\0';
	if (ntime != 0) {
		/*CONSTANTCONDITION*/
		if (sizeof(long) >= 4) {
			char *sp;
			long *dp;
	
			if (fread((void *)&zp->dates[0], 4, ntime, fp) != ntime)
				goto err3;
			dp = &zp->dates[ntime];
			sp = &((char *)(&zp->dates[0]))[4 * ntime];
			n = ntime;
			do
				*--dp = decode(sp -= 4);
			while (--n != 0);
		} else {
			long *p;
	
			p = &zp->dates[0];
			n = ntime;
			do {
				char buf[4];
	
				if (fread((void *)buf, sizeof(buf), 1, fp) != 1)
					goto err3;
				*p++ = decode(buf);
			} while (--n != 0);
		}
		if (fread((void *)&zp->kinds[0], 1, ntime, fp) != ntime) {
		err3:;
			free((void *)zp->zones);
			goto err2;
		} else {
			unsigned char *p = &zp->kinds[0];
	
			do {
				if ((int)*p >= ntype)
					goto err3;
			} while (--ntime);
		}
	}
	/*CONSTANTCONDITION*/
	if (sizeof(struct zone) >= 4 + 1 + 1) {
		unsigned char *sp;
		struct zone *dp;

		if (fread((void *)&zp->zones[0], 4 + 1 + 1, ntype, fp) != ntype)
			goto err3;
		dp = &zp->zones[ntype];
		sp = &((unsigned char *)(&zp->zones[0]))[(4 + 1 + 1) * ntype];
		n = ntype;
		do {
			dp--;
			if ((int)(dp->index = *--sp) >= nchar)
				goto err3;
			if ((dp->isalt = *--sp) > 1)
				dp->isalt = 1;
			dp->offset = -decode((char *)(sp -= 4));
		} while (--n != 0);
	} else {
		struct zone *p;

		p = &zp->zones[0];
		n = ntype;
		do {
			unsigned char buf[4 + 1 + 1];

			if (fread((void *)buf, sizeof(buf), 1, fp) != 1)
				goto err3;
			p->offset = -decode((char *)buf);
			if ((p->isalt = buf[4]) > 1)
				p->isalt = 1;
			if ((int)(p->index = buf[5]) >= nchar)
				goto err3;
		} while (--n != 0);
	}
	if (fread((void *)&zp->strs[0], 1, nchar, fp) != nchar)
		goto err3;
	fclose(fp);
	/*
	* Make typical main (and alternate) timezone handling easier.
	*/
	if (ntype == 1) {
		zp->reg = 0;
		zp->alt = -1;
	} else if (ntype > 2 || zp->zones[0].isalt == zp->zones[1].isalt) {
		zp->reg = -1;
		zp->alt = -1;
	} else if (zp->zones[0].isalt != 0) {
		zp->reg = 1;
		zp->alt = 0;
	} else {
		zp->reg = 0;
		zp->alt = 1;
	}
	return zp;
}

static void
cpy(char *dst, const char *src)
{
	int i = TZNAME_MAX;

	do {
		if ((*dst++ = *src++) == '\0')
			return;
	} while (--i != 0);
	*dst = '\0';
}

static long
add(long t, long off)
{
	if (off > 0) {
		if (t > LONG_MAX - off)
			return LONG_MAX;
	} else if (off < 0) {
		if (t < LONG_MIN - off)
			return LONG_MIN;
	}
	return t + off;
}

main(int argc, char **argv)
{
	int ans;
	long t;
	char *sp;
	char *tz;
	FILE *tzfp;
	struct zone *zp;
	time_t adjustment;
	struct tz_file *fp;
	char buf[BUFSIZ];
	struct tz_info tzbuf;

	t = (long)time((time_t *)0);
	tzbuf.str[0][0] = '\0';
	tzbuf.etc = tzbuf.str[0];

	tz = NULL;
	/*
	 * /etc/TIMEZONE defines the system's timezone.
	 */
	if (access(SYSTZ_FILE, 0) == 0) {
		if ((tzfp = fopen(SYSTZ_FILE, "r")) == NULL) {
			exit(errno);
		}
		while (fgets(buf, sizeof(buf), tzfp) != NULL) {
			if (*buf == '#') {
				continue;
			}
			if (*buf != '\0' && strncmp(buf, "TZ=", 3) == 0) {
				if ((tz = strchr(buf, '\n')) != NULL)
					*tz = '\0';
				tz = strdup(buf);
				(void)putenv(tz);
				tz += 3;
				break;
			}
		}
		(void)fclose(tzfp);
	}
	if (tz == NULL) {
		if ((tz = getenv("TZ")) == NULL) {
			/*
			 * Assume GMT and exit since GMT has no daylight
			 * savings.
			 */
			exit(1);
		}
	}
	/*
	 * Let's figure out if daylight savings exists in this timezone.
	 * The idea is to quietly exit if we're in a single timezone.
	 */
	tzset();
	/*
	 * If this timezone has no daylight savings, quietly exit now.
	 */
	if (daylight == 0) {
		exit(1);
	}

	if (*tz == ':') {
		tzbuf.etc = &tz[1];
	} else {
		tzbuf.etc = &tz[0];
	}
	if ((fp = update(tzbuf.etc)) == NULL) {
		exit(1);
	}
	if (fp->reg >= 0) {
		int lo, hi, mid;

		zp = &fp->zones[fp->reg];
		if (fp->alt < 0) {
			/*
			 * No alternate time in this zone.
			 */
			exit(1);
		}
		zp = &fp->zones[fp->alt];
		/*
		 * Binary search for dates that bracket t.
		 * We care only whether it's the main or alternate.
		 */
		lo = 0;
		hi = fp->count;
		while ((mid = (lo + hi) / 2) != lo) {
			if (t < fp->dates[mid])
				hi = mid;
			else
				lo = mid;
		}
		if (mid + 1 <= fp->count) {
			mid++;
		}
		ans = fp->zones[fp->kinds[mid]].isalt;
		if (ans) {
			zp = &fp->zones[fp->alt];
			adjustment = fp->zones[fp->reg].offset - zp->offset;
		} else {
			zp = &fp->zones[fp->reg];
			adjustment = fp->zones[fp->alt].offset - zp->offset;
		}
		strftime(buf, BUFSIZ, "%R %b %d", localtime(&fp->dates[mid]));
		printf("ADJUSTMENT=%ld\n", adjustment);
		printf("CORRECTION=%ld\n", zp->offset);
		printf("SCHEDDATE=\"%s\"\n", buf);
		printf("TZ_OFFSET=%ld\n", zp->offset);
		exit(0);
	}
	exit(1);
}
