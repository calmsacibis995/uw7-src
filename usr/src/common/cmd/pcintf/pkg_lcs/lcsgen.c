#ident	"@(#)pcintf:pkg_lcs/lcsgen.c	1.3"
/* SCCSID(@(#)lcsgen.c	7.2	LCC)	* Modified: 16:39:39 3/9/92 */
/*
 *  lcsgen - generate a LCS format translation table
 */

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include "lcs.h"
#include "lcs_int.h"


/*
 *  Input format:

$decimal
$hexadecimal
$octal
$default_char xx

$input
xx-yy	direct [double_byte xx-yy] [char_bias xx]
xx-yy	table [double_byte xx-yy]
	xx  [yy]  yyyy
xx-yy	dead_char
	xx  yyyy

$output
xxxx-yyyy	direct_cell [direct_row] [no_lower] [no_upper] [char_bias xx]
xxxx-yyyy	table_4b [no_lower] [no_upper]
	xxxx  yy  [not_exact] [has_multi] xx yy zz [has_2b xx yy]
xxxx-yyyy	table [no_lower] [no_upper]
	xxxx  yy  [not_exact] [has_multi] xx yy zz

*/

char *calloc();
char *strtok();
long lseek();

char *process_multi();
unsigned short get_num();
char *get_range();


lcs_tbl tbl;

int mb_size = 0;

char seps[] = {" \t\r\n"};

int section = 0;	/* section of input: 0 = init, 1 = input, 2 = output */
int radix = 10;
int errs = 0;
int flipBytes;		/* non-zero = flip */

main(argc, argv)
int argc;
char **argv;
{
	FILE *fp;
	char buf[256];

	/* determine machine byte ordering */
	flipBytes = get_byteorder();

	if (argc != 3) {
		printf("usage: lcsgen input output\n");
		exit(1);
	}
	if ((fp = fopen(argv[1], "r")) == NULL) {
		printf("lcsgen: Can't open %s\n", argv[1]);
		perror("");
		exit(2);
	}
	if ((tbl = (lcs_tbl)calloc(1, sizeof(struct table_header))) == NULL) {
		printf("lcsgen: No more space\n");
		exit(1);
	}
	section = 0;
	while (fgets(buf, 256, fp) != NULL) {
		if (section == 0)
			process_init(buf);
		else if (section == 1)
			process_input(buf);
		else  /* section == 2 */
			process_output(buf);
	}
	fclose(fp);
	if (errs == 0)
		write_table(argv[2]);
}

process_init(buf)
char *buf;
{
	register char *bp;

	bp = strtok(buf, seps);
	if (bp == NULL || !strcmp(bp, "$"))
		return;
	if (!stricmp(bp, "$decimal"))
		radix = 10;
	else if (!stricmp(bp, "$hexadecimal"))
		radix = 16;
	else if (!stricmp(bp, "$octal"))
		radix = 8;
	else if (!stricmp(bp, "$input"))
		section = 1;
	else if (!stricmp(bp, "$output"))
		section = 2;
	else if (!stricmp(bp, "$default_char")) {
		if ((bp = strtok(NULL, seps)) != NULL) {
			tbl->th_default = get_num(bp);
			if ((bp = strtok(NULL, seps)) != NULL) {
				tbl->th_default <<= 8;
				tbl->th_default |= 0xff & get_num(bp);
			}
		}
	}
}


process_input(buf)
char *buf;
{
	static struct input_header *ih = NULL;
	struct input_header *ih2;
	unsigned start, end, db_start, db_end;
	unsigned flags, char_bias;
	register char *bp;
	int size;

	if ((bp = strtok(buf, seps)) == NULL || !strcmp(bp, "$"))
		return;
	if (!stricmp(bp, "$output")) {
		section = 2;
		return;
	}
	if (isspace(*buf)) {
		if (ih == NULL) {
			printf("lcsgen: bad input line\n");
			errs++;
			return;
		}
		process_input_table(ih, bp);
		return;
	}
	start = end = db_start = db_end = char_bias = 0;
	ih = NULL;
	bp = get_range(bp, &start, &end);
	if (start == 0xffff)
		return;
	if (bp == NULL && (bp = strtok(NULL, seps)) == NULL) {
		printf("lcsgen: invalid input line\n");
		errs++;
		return;
	}
	if (!stricmp(bp, "direct"))
		flags = IH_DIRECT;
	else if (!stricmp(bp, "table"))
		flags = 0;
	else if (!stricmp(bp, "dead_char"))
		flags = IH_DEAD_CHAR;
	else {
		printf("lcsgen: invalid input line\n");
		errs++;
		return;
	}
	bp = strtok(NULL, seps);
	while (bp != NULL) {
		if (!stricmp(bp, "double_byte")) {
			flags |= IH_DOUBLE_BYTE;
			bp = get_range(strtok(NULL, seps), &db_start, &db_end);
			if (db_start == 0xffff)
				return;
			if (bp != NULL)
				continue;
		} else if (!stricmp(bp, "char_bias"))
			char_bias = get_num(strtok(NULL, seps));
		else {
			printf("lcsgen: Invalid input option: %s\n", bp);
			errs++;
			return;
		}
		bp = strtok(NULL, seps);
	}
	if (flags & IH_DEAD_CHAR)
		size = 512;
	else if (flags & IH_DIRECT)
		size = 0;
	else {	/* table */
		size = sizeof(lcs_char) * (end - start + 1);
		if (flags & IH_DOUBLE_BYTE)
			size *= db_end - db_start + 1;
	}
	if ((ih = (struct input_header *)calloc(1, size +
			sizeof(struct input_header))) == NULL) {
		printf("lcsgen: No more space\n");
		errs++;
		return;
	}
	ih->ih_next = NULL;
	ih->ih_flags = flags;
	ih->ih_start_code = start;
	ih->ih_end_code = end;
	ih->ih_db_start = db_start;
	ih->ih_db_end = db_end;
	ih->ih_char_bias = char_bias;
	if (flags & IH_DEAD_CHAR)
		ih->ih_length = 0;
	if (tbl->th_input == NULL)
		tbl->th_input = ih;
	else {
		for (ih2 = tbl->th_input; ih2->ih_next != NULL; ih2 = ih2->ih_next)
			;
		ih2->ih_next = ih;
	}
	if (flags & IH_DIRECT)
		ih = NULL;
}


process_input_table(ih, bp)
struct input_header *ih;
register char *bp;
{
	unsigned b1, b2, value;
	int i;
	struct input_dead *id;

	b1 = get_num(bp);
	if ((bp = strtok(NULL, seps)) == NULL) {
		printf("lcsgen: Invalid input table format\n");
		errs++;
		return;
	}
	value = b2 = get_num(bp);
	if ((bp = strtok(NULL, seps)) != NULL)
		value = get_num(bp);

	if (ih->ih_flags & IH_DEAD_CHAR) {
		if (ih->ih_length >= 508) {
			printf("lcsgen: Too many dead char table entries\n");
			errs++;
			return;
		}
		id = (struct input_dead *)ih->ih_table;
		id += ih->ih_length / sizeof(struct input_dead);
		ih->ih_length += sizeof(struct input_dead);
		id->id_code = b1;
		id->id_value = value;
	} else {  /* table */
		if (b1 < ih->ih_start_code || b1 > ih->ih_end_code ||
		    ((ih->ih_flags & IH_DOUBLE_BYTE) && (
		     b2 < ih->ih_db_start || b2 > ih->ih_db_end))) {
			printf("lcsgen: Table index out of range: %02x\n", b1);
			return;
		}
		i = b1 - ih->ih_start_code;
		if (ih->ih_flags & IH_DOUBLE_BYTE)
			i += (ih->ih_end_code - ih->ih_start_code + 1) *
			     (b2 - ih->ih_db_start);
		ih->ih_table[i] = value;
	}
}


process_output(buf)
char *buf;
{
	static struct output_header *oh = NULL;
	struct output_header *oh2;
	unsigned start, end;
	unsigned flags, char_bias;
	register char *bp;
	int size;

	if ((bp = strtok(buf, seps)) == NULL || !strcmp(bp, "$"))
		return;
	if (!stricmp(bp, "$input")) {
		section = 1;
		return;
	}
	if (isspace(*buf)) {
		if (oh == NULL) {
			printf("lcsgen: bad output line: %s", buf);
			errs++;
			return;
		}
		process_output_table(oh, bp);
		return;
	}
	start = end = char_bias = 0;
	oh = NULL;
	bp = get_range(bp, &start, &end);
	if (start == 0xffff)
		return;
	if (bp == NULL && (bp = strtok(NULL, seps)) == NULL) {
		printf("lcsgen: invalid output line\n");
		errs++;
		return;
	}
	if (!stricmp(bp, "direct_cell"))
		flags = OH_DIRECT_CELL;
	else if (!stricmp(bp, "table"))
		flags = 0;
	else if (!stricmp(bp, "table_4b"))
		flags = OH_TABLE_4B;
	else {
		printf("lcsgen: invalid output line\n");
		errs++;
		return;
	}
	bp = strtok(NULL, seps);
	while (bp != NULL) {
		if (!stricmp(bp, "direct_row"))
			flags |= OH_DIRECT_ROW;
		else if (!stricmp(bp, "no_lower"))
			flags |= OH_NO_LOWER;
		else if (!stricmp(bp, "no_upper"))
			flags |= OH_NO_UPPER;
		else if (!stricmp(bp, "char_bias"))
			char_bias = get_num(strtok(NULL, seps));
		else {
			printf("lcsgen: Invalid output option: %s\n", bp);
			errs++;
			return;
		}
		bp = strtok(NULL, seps);
	}
	if (flags & OH_DIRECT_CELL)
		size = 0;
	else {	/* table */
		if (flags & (OH_NO_LOWER|OH_NO_UPPER)) {
			unsigned sc, ec;

			size = ((end >> 8) - (start >> 8) - 1) * 0x80;
			sc = start & 0xff;
			ec = end & 0xff;
			if (flags & OH_NO_LOWER) {
				size += (sc <= 0x80) ? 0x80 : 0x100 - sc;
				if (ec >= 0x80)
					size += ec - 0x7f;
			} else {	/* OH_NO_UPPER */
				if (sc <= 0x7f)
					size += 0x80 - sc;
				size += (ec >= 0x7f) ? 0x80 : ec + 1;
			}
		} else
			size = end - start + 1;
		if (flags & OH_TABLE_4B)
			size <<= 2;
		else
			size <<= 1;
	}
	if ((oh = (struct output_header *)calloc(1, size +
			sizeof(struct output_header))) == NULL) {
		printf("lcsgen: No more space\n");
		errs++;
		return;
	}
	oh->oh_next = NULL;
	oh->oh_flags = flags;
	oh->oh_start_code = start;
	oh->oh_end_code = end;
	oh->oh_char_bias = char_bias;
	if (tbl->th_output == NULL)
		tbl->th_output = oh;
	else {
		for (oh2 = tbl->th_output; oh2->oh_next != NULL; oh2 = oh2->oh_next)
			;
		oh2->oh_next = oh;
	}
	if (flags & OH_DIRECT_CELL)
		oh = NULL;
}


process_output_table(oh, bp)
struct output_header *oh;
register char *bp;
{
	unsigned short code, value, b1, b2;
	unsigned short flags;
	int i;
	struct output_table *ot;

	code = get_num(bp);
	if ((bp = strtok(NULL, seps)) == NULL) {
		printf("lcsgen: Invalid output format\n");
		errs++;
		return;
	}
	value = get_num(bp);
	flags = 0;
	bp = strtok(NULL, seps);
	while (bp != NULL) {
		if (!stricmp(bp, "not_exact"))
			flags |= OT_NOT_EXACT;
		else if (!stricmp(bp, "has_multi")) {
			flags |= OT_HAS_MULTI;
			if ((bp = process_multi(code)) != NULL)
				continue;
		} else if ((oh->oh_flags & OH_TABLE_4B) && !stricmp(bp, "has_2b")) {
			flags |= OT_HAS_2B;
			if ((bp = strtok(NULL, seps)) == NULL) {
				printf("lcsgen: Invalid output table format\n");
				errs++;
				return;
			}
			b1 = get_num(bp);
			if ((bp = strtok(NULL, seps)) == NULL) {
				printf("lcsgen: Invalid output table format\n");
				errs++;
				return;
			}
			b2 = get_num(bp);
		} else {
			printf("lcsgen: Invalid output table option: %s\n", bp);
			errs++;
			return;
		}
		bp = strtok(NULL, seps);
	}

	if (code < oh->oh_start_code || code > oh->oh_end_code ||
	    ((oh->oh_flags & OH_NO_LOWER) && ((code & 0x80) == 0)) ||
	    ((oh->oh_flags & OH_NO_UPPER) && (code & 0x80))) {
		printf("lcsgen: Table index out of range: %02x\n", b1);
		errs++;
		return;
	}
	if ((oh->oh_flags & (OH_NO_LOWER|OH_NO_UPPER)) &&
	    ((code & 0xff00) != (oh->oh_start_code & 0xff00)))
		i = ((((code >> 8) - (oh->oh_start_code >> 8)) << 7) +
		     (code & 0xff) - (oh->oh_start_code & 0xff)) << 1;
	else
		i = (code - oh->oh_start_code) << 1;

	if (oh->oh_flags & OH_TABLE_4B) {
		ot = (struct output_table *)&oh->oh_table[i << 1];
		if (flags & OT_HAS_2B) {
			ot->ot_2chars[0] = b1;
			ot->ot_2chars[1] = b2;
		}
	} else
		ot = (struct output_table *)&oh->oh_table[i];
	ot->ot_flags = flags;
	ot->ot_char = value;
}


char *
process_multi(code)
unsigned short code;
{
	unsigned char buf[512];
	unsigned char *b1, *b2;
	struct multi_byte *mb;
	int len;
	char *bp;

	mb = (struct multi_byte *)buf;
	mb->mb_code = code;
	len = 0;
	while ((bp = strtok(NULL, seps)) != NULL) {
		if (*bp == 'n' || *bp == 'N' ||
		    *bp == 'h' || *bp == 'H')
			break;
		mb->mb_text[len++] = get_num(bp);
		if (len > 500) {
			printf("lcsgen: MultiByte string too large\n");
			errs++;
			return bp;
		}
	}
	if (len > 0) {
		mb->mb_length = len;
		if (mb_size == 0)
			tbl->th_multi_byte = (unsigned char *)malloc(mb_size = 512);
		else if (mb_size < (len + tbl->th_multi_length + 4)) {
			mb_size += 512;
			tbl->th_multi_byte = (unsigned char *)
						realloc(tbl->th_multi_byte,
							     mb_size);
		}
		if (tbl->th_multi_byte == NULL) {
			printf("lcsgen: No more memory\n");
			errs++;
			return bp;
		}
		b1 = &tbl->th_multi_byte[tbl->th_multi_length];
		b2 = buf;
		len += 4;
		if (len & 0x03)
			len += 4 - (len & 0x03);
		mb->mb_len = len;
		tbl->th_multi_length += len;
		while (len--)
			*b1++ = *b2++;
	}
	return bp;
}


write_table(file)
char *file;
{
	int fd;
	int hsize;
	char buf[512];
	int i, size;
	struct file_header fhdr;
	struct input_header *ih;
	struct output_header *oh;

	if ((fd = open(file, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0666)) < 0) {
		printf("lcsgen: Can't create %s", file);
		perror("");
		return;
	}
	printf("Writing table %s\n", file);
	strcpy(fhdr.fh_magic, LCS_MAGIC);
	fhdr.fh_num_input_hdrs = 0;
	fhdr.fh_num_output_hdrs = 0;
	fhdr.fh_multi_length = 0;
	fhdr.fh_default = tbl->th_default;
	hsize = sizeof(struct file_header);
	for (ih = tbl->th_input; ih != NULL; ih = ih->ih_next) {
		fhdr.fh_num_input_hdrs++;
		hsize += sizeof(struct input_header);
	}
	for (oh = tbl->th_output; oh != NULL; oh = oh->oh_next) {
		fhdr.fh_num_output_hdrs++;
		hsize += sizeof(struct output_header);
	}
	for (i = 0; i < hsize; i+= 512) {
		size = ((hsize - i) > 512) ? 512 : (hsize - i);
		if (write(fd, buf, size) != size) {
			printf("lcsgen: Output error\n");
			close(fd);
			return;
		}
	}

	lseek(fd, (long)sizeof(struct file_header), 0);
	for (ih = tbl->th_input; ih != NULL; ih = ih->ih_next)
		write_input_table(fd, ih);
	for (oh = tbl->th_output; oh != NULL; oh = oh->oh_next)
		write_output_table(fd, oh);
	if (tbl->th_multi_length > 0) {
		fhdr.fh_multi_length = tbl->th_multi_length;
		fhdr.fh_multi_offset = lseek(fd, 0L, 2);

		if (flipBytes) {
			struct multi_byte *mb;
			unsigned char *bp;
			int l;

			bp = (unsigned char *)tbl->th_multi_byte;
			for (l = fhdr.fh_multi_length; l > 0; ) {
				mb = (struct multi_byte *)bp;
				sflip(mb->mb_code);
				bp += mb->mb_len;
				l -= mb->mb_len;
			}
		}

		if (write(fd, tbl->th_multi_byte, tbl->th_multi_length) !=
		    tbl->th_multi_length) {
			printf("lcsgen: Output error\n");
			close(fd);
			return;
		}
	}
	if (flipBytes) {
	    sflip(fhdr.fh_num_input_hdrs);
	    sflip(fhdr.fh_num_output_hdrs);
	    lflip(fhdr.fh_multi_offset);
	    sflip(fhdr.fh_multi_length);
	    sflip(fhdr.fh_default);
	}
	lseek(fd, 0L, 0);
	if (write(fd, &fhdr, sizeof(fhdr)) != sizeof(fhdr))
		printf("lcsgen: Output error\n");
	close(fd);
}

write_input_table(fd, ih)
int fd;
struct input_header *ih;
{
	long fpos;
	struct input_header ihdr;
	int size;

	if (ih->ih_flags & IH_DIRECT)
		ihdr.ih_offset = 0L;
	else {
		fpos = lseek(fd, 0L, 1);
		ihdr.ih_offset = lseek(fd, 0L, 2);
		if (ih->ih_flags & IH_DEAD_CHAR)
			size = ih->ih_length;
		else {	/* table */
			size = sizeof(lcs_char) *
				(ih->ih_end_code - ih->ih_start_code + 1);
			if (ih->ih_flags & IH_DOUBLE_BYTE)
				size *= ih->ih_db_end - ih->ih_db_start + 1;
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

		if (write(fd, ih->ih_table, size) != size) {
			printf("lcsgen: Output error\n");
			exit(1);
		}
		lseek(fd, fpos, 0);
	}
	ihdr.ih_flags = ih->ih_flags;
	ihdr.ih_start_code = ih->ih_start_code;
	ihdr.ih_end_code = ih->ih_end_code;
	ihdr.ih_db_start = ih->ih_db_start;
	ihdr.ih_db_end = ih->ih_db_end;
	if (ih->ih_flags & IH_DEAD_CHAR) {
		ihdr.ih_length = ih->ih_length;
		if (flipBytes)
		    sflip(ihdr.ih_length);
	}
	ihdr.ih_char_bias = ih->ih_char_bias;
	ihdr.ih_table[0] = 0;
	ihdr.ih_table[1] = 0;
	if (flipBytes) {
	    lflip(ihdr.ih_un.ihu_offset);
	    sflip(ihdr.ih_flags);
	    sflip(ihdr.ih_char_bias);
	}
	if (write(fd, &ihdr, sizeof(ihdr)) != sizeof(ihdr)) {
		printf("lcsgen: Output error\n");
		exit(1);
	}
}


write_output_table(fd, oh)
int fd;
struct output_header *oh;
{
	long fpos;
	struct output_header ohdr;
	int size;

	if (oh->oh_flags & OH_DIRECT_CELL)
		ohdr.oh_offset = 0L;
	else {
		fpos = lseek(fd, 0L, 1);
		ohdr.oh_offset = lseek(fd, 0L, 2);
		if (oh->oh_flags & (OH_NO_LOWER|OH_NO_UPPER)) {
			unsigned sc, ec;

			size = ((oh->oh_end_code >> 8) -
				(oh->oh_start_code >> 8) - 1) * 0x80;
			sc = oh->oh_start_code & 0xff;
			ec = oh->oh_end_code & 0xff;
			if (oh->oh_flags & OH_NO_LOWER) {
				size += (sc <= 0x80) ? 0x80 : 0x100 - sc;
				if (ec >= 0x80)
					size += ec - 0x7f;
			} else {	/* OH_NO_UPPER */
				if (sc <= 0x7f)
					size += 0x80 - sc;
				size += (ec >= 0x7f) ? 0x80 : ec + 1;
			}
		} else
			size = oh->oh_end_code - oh->oh_start_code + 1;
		if (oh->oh_flags & OH_TABLE_4B)
			size <<= 2;
		else
			size <<= 1;

		if (write(fd, oh->oh_table, size) != size) {
			printf("lcsgen: Output error\n");
			exit(1);
		}
		lseek(fd, fpos, 0);
	}
	ohdr.oh_flags = oh->oh_flags;
	ohdr.oh_start_code = oh->oh_start_code;
	ohdr.oh_end_code = oh->oh_end_code;
	ohdr.oh_char_bias = oh->oh_char_bias;
	ohdr.oh_table[0] = 0;
	ohdr.oh_table[1] = 0;
	ohdr.oh_table[2] = 0;
	ohdr.oh_table[3] = 0;
	if (flipBytes) {
	    lflip(ohdr.oh_un.ohu_offset);
	    sflip(ohdr.oh_flags);
	    sflip(ohdr.oh_start_code);
	    sflip(ohdr.oh_end_code);
	    sflip(ohdr.oh_char_bias);
	}
	if (write(fd, &ohdr, sizeof(ohdr)) != sizeof(ohdr)) {
		printf("lcsgen: Output error\n");
		exit(1);
	}
}


unsigned short
get_num(bp)
register char *bp;
{
	unsigned res;

	res = 0;
	if (bp == NULL) {
		printf("lcsgen: Invalid number\n");
		errs++;
		return res;
	}
	while (*bp) {
		res *= radix;
		if (*bp >= '0' && *bp <= '7')
			res += *bp++ - '0';
		else if (radix > 8 && *bp >= '8' && *bp <= '9')
			res += *bp++ - '0';
		else if (radix == 16 && *bp >= 'a' && *bp <= 'f')
			res += *bp++ - 'a' + 10;
		else if (radix == 16 && *bp >= 'A' && *bp <= 'F')
			res += *bp++ - 'A' + 10;
		else {
			printf("lcsgen: Invalid digit (%c)\n", *bp++);
			errs++;
		}
	}
	return res;
}


char *
get_range(buf, sp, ep)
char *buf;
unsigned *sp, *ep;
{
	char *bp;
	int found_dash;

	found_dash = 0;
	for (bp = buf; *bp && *bp != '-'; bp++)
		;
	if (*bp) {
		*bp++ = '\0';
		found_dash++;
	}
	*ep = *sp = get_num(buf);
	if (!found_dash) {
		bp = strtok(NULL, seps);
		if (bp == NULL || *bp != '-')
			return bp;
		bp++;
	}
	if (*bp == '\0' &&
	    (bp = strtok(NULL, seps)) == NULL) {
		printf("lcsgen: Invalid range\n");
		*sp = 0xffff;
		errs++;
		return NULL;
	}
	*ep = get_num(bp);
	return NULL;
}

/*
 *  int get_byteorder()
 *
 *	This function determines the machine byte ordering.
 *	Returns 0 if flipping is not necessary.
 *	Returns 1 if flipping is necessary (i.e. big endian architecture).
 */
int
get_byteorder()
{
	int	ivar = 1;

	if (*(char *)&ivar)
		return 0;
	else 
		return 1;
}
