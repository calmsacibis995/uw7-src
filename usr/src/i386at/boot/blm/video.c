#ident	"@(#)stand:i386at/boot/blm/video.c	1.3"
#ident	"$Header$"

/*
 * Image file handling (platform-specific).
 */

#include <boot.h>
#include <bioscall.h>
#include <pcx.h>

STATIC void set_mode(enum display_mode mode);
STATIC int display_image(struct pcx_header *hdrp, unsigned char *datap);

STATIC struct L_video _L_video = {
	set_mode, display_image
};

/* Display size, for 640 X 480 16-color VGA */
#define BITSPERPIXEL	1
#define NPLANES		4
#define NCOLS		640
#define ROWSIZE		((NCOLS * BITSPERPIXEL) / 8)
#define NROWS		480

STATIC int original_mode = -1;
STATIC unsigned char *rowbuf;
STATIC unsigned char palette[16*3];

STATIC void set_palette(void);


void
video_start(void)
{
	L_video = &_L_video;
}

#if 0
/*
 * See if VGA can be put into graphics mode. Return non-zero on success.
 */
STATIC int
probe_video(void)
{
	struct biosregs regs, orig_regs;
	unsigned char resulting_mode;

	regs.intnum = orig_regs.intnum = 0x10;	/* video BIOS functions */

	/* Save original video mode */
	orig_regs.ax = 0x0f00;		/* get current video mode (and page) */
	(void)bioscall(&orig_regs);
	original_mode = orig_regs.ax & 0x7f; /* (bit 0x80 = no clear screen) */

	/* Save cursor position */
	orig_regs.ax = 0x0300;		/* get cursor position and size */
	(void)bioscall(&orig_regs);	/* (BH still set from get mode call) */

	/* Set to the desired mode */
	regs.ax = 0x0092;	/* VGA mode 0x12, 640x480 16 colors, no clear */
	(void)bioscall(&regs);

	/* Read back current mode and see if we successfully changed it */
	regs.ax = 0x0f00;		/* get current video mode */
	(void)bioscall(&regs);
	resulting_mode = regs.ax & 0x7f;

	/* Restore original mode (w/o clearing the screen) */
	regs.ax = original_mode | 0x80;
	(void)bioscall(&regs);

	/* Restore cursor position */
	orig_regs.ax = 0x0200;		/* set cursor position */
	(void)bioscall(&orig_regs);

	return (resulting_mode == 0x12);
}
#endif

/*
 * Set the VGA display into text or graphics mode.
 */
STATIC void
set_mode(enum display_mode mode)
{
	static struct biosregs regs;
	int orig_mode;

	regs.intnum = 0x10;	/* video BIOS functions */

	switch (mode) {
	case TEXT:
		ASSERT(original_mode != -1);
		regs.ax = original_mode;	/* set mode to original mode */
		break;
	case GRAPHICS:
		if (original_mode == -1) {
			/* First time; save original video mode */
			regs.ax = 0x0F00;	/* get current video mode */
			(void)bioscall(&regs);
			orig_mode = regs.ax & 0x7F;
		}
		memzero(palette, sizeof palette);
		regs.ax = 0x0012;	/* VGA mode 0x12, 640x480 16 colors */
		break;
	}

	(void)bioscall(&regs);		/* switch modes */

	if (original_mode == -1) {
		/* First time: make sure switch to graphics worked. */
		regs.ax = 0x0F00;	/* get current video mode */
		(void)bioscall(&regs);
		if ((regs.ax & 0x7F) != 0x12) {
			/* Unlink from L_video, so we don't get called again. */
			L_video = NULL;
			return;
		}

		/* One-time initialization. */
		original_mode = orig_mode;
		rowbuf = malloc(NPLANES * ROWSIZE);
	}
}

/*
 * Display a VGA image, 640 x 480 16 colors, from PCX format.
 *
 * NOTE: All use of 0xC0 as a special prefix code are non-standard extensions.
 */
STATIC int
display_image(struct pcx_header *hdrp, unsigned char *datap)
{
	unsigned char *vidbuf, *row;
	unsigned char x;
	int count, y, n, plane, plane_no, bplpp, yend, nplanes, rowoff;
	int nsame;

	if ((yend = hdrp->Ymax + 1) > NROWS)
		yend = NROWS;
	if ((y = hdrp->Ymin) >= yend ||
	    (rowoff = hdrp->Xmin / 8) >= ROWSIZE)
		return 0;
	if ((bplpp = hdrp->Bytes_per_line_per_plane) > ROWSIZE - rowoff)
		bplpp = ROWSIZE - rowoff;
	if ((nplanes = hdrp->Nplanes) > NPLANES)
		nplanes = NPLANES;

	/*
	 * Check if the color palette has changed and update it if we can.
	 */
	if (memcmp(palette, hdrp->ColorMap, sizeof palette) != 0) {
		/*
		 * We can't call BIOS recursively, so defer display of this
		 * frame if we're in an I/O wait and we would need to set
		 * the palette.
		 */
		if (biowait)
			return -1;
		memcpy(palette, hdrp->ColorMap, sizeof palette);
		set_palette();
	}

	vidbuf = (unsigned char *)0x000A0000 + y * ROWSIZE + rowoff;

	for (nsame = 0; y < yend; y++) {
		row = rowbuf;
		plane = 0x0100;
		if (nsame)
			--nsame;
		if (nsame == 0 && *datap == 0xC0 && (nsame = datap[1]) != 0)
			datap += 2;
		for (plane_no = 0; plane_no < nplanes; plane_no++) {
			/*
			 * Fill in rowbuf[] w/pixels for this plane of
			 * this row.
			 */
			if (nsame != 0)
				goto got_row;
			if (*datap == 0xC0) {
				/* Process special row codes. */
				++datap;
				switch (*datap++) {
				case 0: /* xor row */
					break;
				case 1: /* same as previous plane */
					memcpy(row, row - ROWSIZE, bplpp);
					goto got_row;
				}
			} else
				memzero(row, bplpp);
			for (n = 0; n < bplpp;) {
				x = *datap++;
				count = 1;
				if ((x & 0xC0) == 0xC0) {
					count = x & 0x3F;
					if (n + count > bplpp)
						count = bplpp - n;
					x = *datap++;
				}
				while (count-- != 0)
					row[n++] ^= x;
			}
got_row:
			/* load map mask register w/"plane" */
			outw(0x3C4, plane + 2);
			/* copy row (for this plane) to frame buffer */
			memcpy(vidbuf, row, bplpp);
			row += ROWSIZE;
			/* move on to a higher plane */
			plane <<= 1;
		}
		/* move on to next row in frame buffer */
		vidbuf += ROWSIZE;
	}

	return 0;
}

/*
 * Set VGA color palette.
 */
STATIC void
set_palette(void)
{
	struct biosregs regs;
	unsigned char r, g, b, *mapp;
	int pr;

	regs.intnum = 0x10;	/* video BIOS functions */

	mapp = palette;
	for (pr = 0; pr < 16; pr++) {
		/*
		 * Compute color value, color, for palette register, pr.
		 */
		r = *mapp++;
		g = *mapp++;
		b = *mapp++;

		/* Use a one-to-one mapping to DAC registers. */
		regs.ax = 0x1000;	/* set single palette register */
		regs.bx = (pr << 8) | pr;
		(void)bioscall(&regs);

		/* Put the actual color into the DAC. */
		regs.dx = ((r >> 2) << 8);
		regs.cx = ((g >> 2) << 8) | (b >> 2);

		regs.ax = 0x1010;	/* set individual DAC register */
		regs.bx = pr;
		(void)bioscall(&regs);
	}

	/* Force border to black. */
	regs.ax = 0x1010;	/* set individual DAC register */
	regs.bx = 16;
	regs.cx = regs.dx = 0;
	(void)bioscall(&regs);
	regs.ax = 0x1001;	/* set border (overscan) color */
	regs.bx = 16 << 8;
	(void)bioscall(&regs);
}
