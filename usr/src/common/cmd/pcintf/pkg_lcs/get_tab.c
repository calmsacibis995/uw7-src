#ident	"@(#)pcintf:pkg_lcs/get_tab.c	1.3"
/* SCCSID(@(#)get_tab.c	7.2	LCC)	* Modified: 16:34:58 3/9/92 */
/*
 *  lcs_get_table(table, lcspath_default)
 *
 *  Load a translation table
 */

#define NO_LCS_EXTERNS

#include <fcntl.h>
#include "lcs.h"
#include "lcs_int.h"


/*
 *  Define library variables
 */

int	lcs_errno;
lcs_tbl lcs_input_table = NULL;
lcs_tbl lcs_output_table = NULL;

static int flipBytes = 0;		/* non-zero = flip */

char *getenv();
long  lseek();
char *malloc();


lcs_tbl
lcs_get_table(table, lcspath_default)
char *table;
char *lcspath_default;
{
	char buf[256];
	char *bp;
	char *lcspath;
	int fd;
	struct file_header fh;
	struct table_header *th;

	/* determine machine byte ordering */
	flipBytes = lcs_byteorder();

	if ((lcspath = getenv("LCSPATH")) == NULL)
		lcspath = lcspath_default;
again:
	if (lcspath != NULL && *lcspath != '\0') {
		strcpy(buf, lcspath);
		for (bp = buf; *bp; bp++) {
#ifdef MSDOS
			if (*bp == ';')
#else
			if (*bp == ':')
#endif
				break;
		}
		lcspath += bp - buf;
		if (*lcspath != '\0')
			lcspath++;
		*bp++ = '/';
		*bp = '\0';
	}

	strcat(buf, table);
	strcat(buf, ".lcs");
	if ((fd = open(buf, O_RDONLY|O_BINARY)) < 0) {
		if (lcspath != NULL && *lcspath != '\0')
			goto again;
		lcs_errno = LCS_ERR_NOTFOUND;
		return NULL;
	}
	if ((read(fd, &fh, sizeof (fh)) != sizeof(fh)) ||
	    strcmp(fh.fh_magic, LCS_MAGIC) != 0) {
		close(fd);
		lcs_errno = LCS_ERR_BADTABLE;
		return NULL;
	}
	if (flipBytes) {
	    sflip(fh.fh_num_input_hdrs);
	    sflip(fh.fh_num_output_hdrs);
	    lflip(fh.fh_multi_offset);
	    sflip(fh.fh_multi_length);
	    sflip(fh.fh_default);
	}
	if ((th = (struct table_header *)malloc(sizeof (struct table_header))) == NULL) {
		close(fd);
		lcs_errno = LCS_ERR_NOSPACE;
		return NULL;
	}
	th->th_input = NULL;
	th->th_output = NULL;
	th->th_multi_byte = NULL;
	th->th_multi_length = 0;
	th->th_default = fh.fh_default;
	strcpy(th->th_magic, LCS_MAGIC);

	while (fh.fh_num_input_hdrs--) {
		if (lcs_read_input_table(fd, th) < 0) {
			lcs_release_table(th);
			close(fd);
			/* lcs_errno is already set */
			return NULL;
		}
	}
	while (fh.fh_num_output_hdrs--) {
		if (lcs_read_output_table(fd, th) < 0) {
			lcs_release_table(th);
			close(fd);
			/* lcs_errno is already set */
			return NULL;
		}
	}
	if (fh.fh_multi_length > 0) {
		if ((bp = (char *)malloc(fh.fh_multi_length)) == NULL) {
			lcs_release_table(th);
			close(fd);
			lcs_errno = LCS_ERR_NOSPACE;
			return NULL;
		}
		lseek(fd, fh.fh_multi_offset, 0);
		if (read(fd, bp, fh.fh_multi_length) != fh.fh_multi_length) {
			lcs_release_table(th);
			close(fd);
			lcs_errno = LCS_ERR_BADTABLE;
			return NULL;
		}
		th->th_multi_byte = (unsigned char *)bp;
		th->th_multi_length = fh.fh_multi_length;

		if (flipBytes) {
			struct multi_byte *mb;
			int l;

			for (l = fh.fh_multi_length; l > 0; ) {
				mb = (struct multi_byte *)bp;
				sflip(mb->mb_code);
				bp += mb->mb_len;
				l -= mb->mb_len;
			}
		}

	}
	close(fd);
	lcs_errno = 0;
	return th;
}


/*
 *  lcs_release_table(tbl)
 *	lcs_tbl tbl;
 *
 *  Release the storage used by a table
 *
 */

lcs_release_table(tbl)
struct table_header *tbl;
{
	struct input_header *ih, *nih;
	struct output_header *oh, *noh;

	if (strcmp(tbl->th_magic, LCS_MAGIC)) {
		lcs_errno = LCS_ERR_BADTABLE;
		return -1;
	}

	/* Check for set tables */
	if (tbl == lcs_input_table)
		lcs_input_table = NULL;
	if (tbl == lcs_output_table)
		lcs_output_table = NULL;

	tbl->th_magic[0] = 'X';		/* invalidate table */
	for (ih = tbl->th_input; ih; ih = nih) {
		nih = ih->ih_next;
		free(ih);
	}
	for (oh = tbl->th_output; oh; oh = noh) {
		noh = oh->oh_next;
		free(oh);
	}
	if (tbl->th_multi_byte)
		free(tbl->th_multi_byte);
	free(tbl);
	return 0;
}


lcs_read_input_table(fd, th)
int fd;
struct table_header *th;
{
	struct input_header ihdr, *ih, *ihnext;
	long fpos;
	int size;

	if (read(fd, &ihdr, sizeof(ihdr)) != sizeof(ihdr)) {
		lcs_errno = LCS_ERR_BADTABLE;
		return -1;
	}
	if (flipBytes) {
	    lflip(ihdr.ih_un.ihu_offset);
	    sflip(ihdr.ih_flags);
	    sflip(ihdr.ih_char_bias);
	}

	size = 0;
	if (ihdr.ih_flags & IH_DEAD_CHAR) {
		if (flipBytes)
		    sflip(ihdr.ih_length);
		size = ihdr.ih_length;
	} else if ((ihdr.ih_flags & IH_DIRECT) == 0) {
		size = sizeof(lcs_char)*(ihdr.ih_end_code-ihdr.ih_start_code+1);
		if (ihdr.ih_flags & IH_DOUBLE_BYTE)
			size *= ihdr.ih_db_end - ihdr.ih_db_start + 1;
	}
	if ((ih = (struct input_header *)malloc(size + sizeof(ihdr))) == NULL) {
		lcs_errno = LCS_ERR_NOSPACE;
		return -1;
	}
	ih->ih_next = NULL;
	ih->ih_flags = ihdr.ih_flags;
	ih->ih_start_code = ihdr.ih_start_code;
	ih->ih_end_code = ihdr.ih_end_code;
	ih->ih_length = ihdr.ih_length;	      /* also ih_db_start, ih_db_end */
	ih->ih_char_bias = ihdr.ih_char_bias;
	if (size) {
		fpos = lseek(fd, 0L, 1);
		lseek(fd, ihdr.ih_offset, 0);
		if (read(fd, ih->ih_table, size) != size) {
			free(ih);
			lcs_errno = LCS_ERR_BADTABLE;
			return -1;
		}

		if (flipBytes) {
		    if (ihdr.ih_flags & IH_DEAD_CHAR) {
			struct input_dead *id;
			int l;

			id = (struct input_dead *)ih->ih_table;
			for (l = ih->ih_length; l > 0;
			     l -= sizeof(struct input_dead)) {
				sflip(id->id_value);
				id++;
			}
		    } else {
			int i;

			for (i = 0; i < size/2; i++)
				sflip(ih->ih_table[i]);
		    }
		}

		lseek(fd, fpos, 0);
	}
	if (th->th_input == NULL)
		th->th_input = ih;
	else {
		for (ihnext = th->th_input;
		     ihnext->ih_next != NULL;
		     ihnext = ihnext->ih_next)
			;
		ihnext->ih_next = ih;
	}
	return 0;
}


lcs_read_output_table(fd, th)
int fd;
struct table_header *th;
{
	struct output_header ohdr, *oh, *ohnext;
	long fpos;
	int size;
	int sc, ec;

	if (read(fd, &ohdr, sizeof(ohdr)) != sizeof(ohdr)) {
		lcs_errno = LCS_ERR_BADTABLE;
		return -1;
	}
	if (flipBytes) {
	    lflip(ohdr.oh_un.ohu_offset);
	    sflip(ohdr.oh_flags);
	    sflip(ohdr.oh_start_code);
	    sflip(ohdr.oh_end_code);
	    sflip(ohdr.oh_char_bias);
	}
	size = 0;
	if ((ohdr.oh_flags & OH_DIRECT_CELL) == 0) {
		if (ohdr.oh_flags & (OH_NO_LOWER|OH_NO_UPPER)) {
			size = ((ohdr.oh_end_code >> 8) -
				(ohdr.oh_start_code >> 8) - 1) * 0x80;
			sc = ohdr.oh_start_code & 0xff;
			ec = ohdr.oh_end_code & 0xff;
			if (ohdr.oh_flags & OH_NO_LOWER) {
				size += (sc <= 0x80) ? 0x80 : 0x100 - sc;
				if (ec >= 0x80)
					size += ec - 0x7f;
			} else {	/* OH_NO_UPPER */
				if (sc <= 0x7f)
					size += 0x80 - sc;
				size += (ec >= 0x7f) ? 0x80 : ec + 1;
			}
		} else
			size = ohdr.oh_end_code - ohdr.oh_start_code + 1;
		if (ohdr.oh_flags & OH_TABLE_4B)
			size <<= 2;
		else
			size <<= 1;
	}
	if ((oh = (struct output_header *)malloc(size+sizeof(ohdr))) == NULL) {
		lcs_errno = LCS_ERR_NOSPACE;
		return -1;
	}
	oh->oh_next = NULL;
	oh->oh_flags = ohdr.oh_flags;
	oh->oh_start_code = ohdr.oh_start_code;
	oh->oh_end_code = ohdr.oh_end_code;
	oh->oh_char_bias = ohdr.oh_char_bias;
	if (size) {
		fpos = lseek(fd, 0L, 1);
		lseek(fd, ohdr.oh_offset, 0);
		if (read(fd, oh->oh_table, size) != size) {
			free(oh);
			lcs_errno = LCS_ERR_BADTABLE;
			return -1;
		}
		lseek(fd, fpos, 0);
	}
	if (th->th_output == NULL)
		th->th_output = oh;
	else {
		for (ohnext = th->th_output;
		     ohnext->oh_next != NULL;
		     ohnext = ohnext->oh_next)
			;
		ohnext->oh_next = oh;
	}
	return 0;
}

/*
 *  int lcs_byteorder()
 *
 *	This function determines the machine byte ordering.
 *	Returns 0 if flipping is not necessary.
 *	Returns 1 if flipping is necessary (i.e. big endian architecture).
 */
int
lcs_byteorder()
{
	int	ivar = 1;

	if (*(char *)&ivar)
		return 0;
	else 
		return 1;
}
