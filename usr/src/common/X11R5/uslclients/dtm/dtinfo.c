#ifndef NOIDENT
#ident	"@(#)dtm:dtinfo.c	1.15"
#endif

#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <X11/Intrinsic.h>
#include <mapfile.h>
#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"


/*
 * .dtinfo file format:
 *
 */
int
DmReadDtInfo(cp, filepath, options)
DmContainerPtr cp;
char *filepath;
DtAttrs options;
{
	DmMapfilePtr mp;

	/* assume dot file doesn't exist (in case of error) */

	if (!(mp = Dm__mapfile(filepath, PROT_READ, MAP_SHARED))) {

		/*
		 * may failed, because of permission, or other environmental
		 * differences between the last modifier of the file and the
		 * current reader. So just ignore the error silently.
		 */
		return(-1);
	}

	/* get view options */
	while (MF_NOT_EOF(mp)) {
		char *save;
		char *eol;
		char *line;
		char *equal;

		if (MF_PEEKC(mp) == '\037') {
			/* got a view options */
			MF_NEXTC(mp); /* skip '^A' */
			save = MF_GETPTR(mp);
			eol = Dm__findchar(mp, '\n');
			if (MF_GETPTR(mp) == NULL)
				goto bye;
			line = strndup(save, eol - save);
			equal = strchr(line, '=');
			if (equal == NULL)
				goto bye;
			*equal++ = '\0';
			DtSetProperty(&(cp->plist), line, equal, 0);
			free(line);
			MF_NEXTC(mp);
		}
		else
			break;
	}

	while (MF_NOT_EOF(mp)) {
		char *name;	/* ptr to name */
		char *value;	/* ptr to value */
		char *sep;	/* ptr to separator char */
		char *p;	/* tmp var */
		int namelen;	/* length of name */
		int vallen;	/* length of value */
		int i;
		register DmObjectPtr op;

		/*
		 * Process each line with the format:
		 *	filename^Ax,y{name=value;...}
		 *
		 * the curly bracket list of properties is optional.
		 */

		/* get filename. End of name is right before a tab char */
		name = MF_GETPTR(mp);
		if (!(sep = Dm__findchar(mp, '\037')))
			goto bye;
		namelen = sep - name;
		MF_NEXTC(mp); /* skip '\037' */

		if (options & INTERSECT) {
			/*
		 	* See if we can find the name in the current file
			* entries. If not, don't bother processing the
			* rest of the line, simply fast forward to newline.
		 	*/
			for (op=cp->op; op; op=op->next) {
				if ((strlen(op->name) == namelen) &&
			    	(!strncmp(op->name, name, namelen)))
					break;
			}
		}
		else {
			if ((op = Dm__NewObject(cp, NULL)) == NULL)
				continue; /* no memory? */
			op->name = strndup(name, namelen);
		}

		if (op) {
			/* found a matching file entry */
			/* get x,y */
			value = MF_GETPTR(mp);
			if (!(sep = Dm__findchar(mp, ',')))
				goto skip_line;
			p = strndup(value, sep - value);
			op->x = atoi(p);
			free(p);
			MF_NEXTC(mp); /* skip ',' */
			value = MF_GETPTR(mp);
			if (!(sep = Dm__strchr(mp, "{\n"))) {
				op->x = 0; /* reset x,y to 0 */
				goto skip_line;
			}
			p = strndup(value, sep - value);
			op->y = atoi(p);
			free(p);
			if (MF_PEEKC(mp) == '\n')
				goto skip_line;
			MF_NEXTC(mp); /* skip '{' */

			/* check curly bracket */
			while (MF_NOT_EOF(mp) && (MF_PEEKC(mp) != '}')) {
				char *findchars;
				DtAttrs attrs;

				name = MF_GETPTR(mp);
				if (!(p = Dm__strchr(mp, "(=")))
					goto skip_line;

				if (*p == '(') {
					/* process optional attributes */
					char *p2;

					if (!(p2 = Dm__findchar(mp, ')')))
						goto skip_line;

					p++;
					p2 = strndup(p, p2 - p);
					attrs = atol(p2);
					free(p2);

					MF_NEXTC(mp); /* skip ')' */
					if (MF_PEEKC(mp) != '=')
						goto skip_line;

					p--; /* move ptr back to '(' */
				}
				else
					attrs = 0;
				namelen = (int)(p - name);
				MF_NEXTC(mp); /* skip '=' */
				if ((value = MF_GETPTR(mp)) == NULL)
					goto bye;
				if (*value == '"') {
					findchars = "\"";
					value++;
					MF_NEXTC(mp); /* skip '"' */
				}
				else
					findchars = ";}";
				if (!(p = Dm__strchr(mp, findchars)))
					goto skip_line;

				vallen = (int)(p - value);
				if (*findchars == '"')
					MF_NEXTC(mp); /* skip '"' */
				if (namelen && vallen) {
					name = strndup(name, namelen);
					value = strndup(value, vallen);
					DtSetProperty(&(op->plist),
						name, value, attrs);
					free(value);
					free(name);
				}
				else
					goto skip_line;

				if (MF_PEEKC(mp) == '}') {
					/* end of entry */
					goto skip_line;
				}
				else {
					MF_NEXTC(mp); /* skip ';' */
				}
			}
		}
		else {
skip_line:
			/* so skip to the next line */
			(void)Dm__findchar(mp, '\n');
			MF_NEXTC(mp); /* skip '\n' */
		}
	} /* while !EOF */

bye:
	Dm__unmapfile(mp);
	return(0);
}

void
DmWriteDtInfo(cp, filepath, options)
DmContainerPtr cp;
char *filepath;
DtAttrs options;
{
	int fd;
	FILE *file;
	DmObjectPtr op;
	DtPropPtr pp;
	extern int errno;
	char buf[1024];

	if (!(options & DM_B_PERM)) {
		if ((cp->num_objs == 0) && (cp->plist.count == 0)) {
			/* nothing to write out, so remove the file */
			(void)unlink(filepath);
			return;
		}
	}

	if ((fd = open(filepath, O_WRONLY | O_TRUNC | O_CREAT,
			 (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
			  S_IROTH | S_IWOTH))) == -1) {
		/* ignore the error silently */
		return;
	}

	if (lockf(fd, F_LOCK, 0L) == -1) {
err:
		/*
		 * Ignore lock errors. Some file system, such as /stand and
		 * /proc, do not support such operation.
		 */
		/* Dm__VaPrintMsg(TXT_LOCK_FILE, filepath, errno); */
		close(fd) ;
		return;
	}

	if ((file = fdopen(fd, "w")) == NULL)
		goto err;

	/* write out view options */
	pp = DtFindProperty(&(cp->plist), 0);
	while (pp) {
		fprintf(file, "\037%s=%s\n", pp->name, pp->value);
		pp = DtFindProperty(NULL, 0);
	}

	for(op=cp->op; op; op=op->next) {
		fprintf(file, "%s\037%d,%d", op->name, op->x, op->y);
		if (op->plist.count) {
			(void)DtPropertyListToStr(buf, &(op->plist));
			fprintf(file, "%s", buf);
		}
		putc('\n', file);
	}

	fclose(file);
}

