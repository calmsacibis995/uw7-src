#ident	"@(#)vidi.c	1.1"
/*
 *	Copyright (C) The Santa Cruz Operation, 1988.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/***	vidi -- set video options
 *
 *	vidi [ - (no flags yet)] [ options ]
 *
 */
#pragma comment(exestr, "@(#) vidi.c 22.1 89/10/17 ")

#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <sys/kd.h>

extern int errno;

struct mds {
    char *string;
    int	set;
    int	reset;
};

int	extract=0;	/* extract a font if set, otherwise load */
char	*filename=0;

struct vid_tab {
    char *name;
    int mode_code;
}
    vid_tab[]= {
    {"mono",		SWAPMONO},	/* swap commands */
    {"cga",		SWAPCGA},
    {"ega",		SWAPEGA},
    {"vga",		SWAPVGA},

    {"c40x25",		SW_C40x25},	/* 40x25 modes */
    {"e40x25",		SW_ENHC40x25},
    {"v40x25",		SW_VGA40x25},

    {"m80x25",		SW_B80x25},	/* 80x25 modes */
    {"c80x25",		SW_C80x25},
    {"em80x25",		SW_EGAMONO80x25},
    {"e80x25",		SW_ENHC80x25},
    {"vm80x25",		SW_VGAM80x25},
    {"v80x25",		SW_VGA80x25},

    {"e80x43",		SW_ENHC80x43},	/* 80x43 modes */

    {"mode5",		SW_CG320},	/* graphics modes */
    {"mode6",		SW_BG640},
    {"modeD",		SW_CG320_D},
    {"modeE",		SW_CG640_E},
    {"modeF",		SW_ENH_MONOAPA2},
    {"mode10",		SW_ENH_CG640},
    {"mode11",		SW_VGA11},
    {"mode12",		SW_VGA12},
    {"mode13",		SW_VGA13},
    {"internal",	INTERNAL_VID},		/* S000 */
    {"external",	EXTERNAL_VID},		/* S000 */
	/* Enhanced Application Compatability */
    {"att640", 		SW_ATT640},
    {"att800x600", 	SW_VDC800x600E},
    {"att640x400",	SW_VDC640x400V},
	/* End  Enhanced Application Compatability */
};

struct {
	char *name;
	int get_font, set_font, fontsize;
}
    font_tab[]= {
	{"font8x8",	GIO_FONT8x8,	PIO_FONT8x8,	8*256},
	{"font8x14",	GIO_FONT8x14,	PIO_FONT8x14,	14*256},
	{"font8x16",	GIO_FONT8x16,	PIO_FONT8x16,	16*256},
};

char	VIDI[]	= "vidi";
char	USAGE[]	= "usage: vidi [-f fontfile] [-d] [mode or font]";

#define USAGE_COLS 5
void		/* doesn't return */
usage() {
    int idx;

    fprintf(stderr, "%s\n", USAGE);

    printf("modes:\n");
    for(idx=0; idx<sizeof(vid_tab)/sizeof(*vid_tab); idx++ ) {
	printf("%-15s", vid_tab[idx].name);
	if ( (idx % USAGE_COLS) == USAGE_COLS-1)
	    printf("\n");
    }
    if ( idx % USAGE_COLS )
	printf("\n");

    printf("\nfonts:\n");
    for(idx=0; idx<sizeof(font_tab)/sizeof(*font_tab); idx++ ) {
	printf("%-15s", font_tab[idx].name);
	if ( (idx % USAGE_COLS) == USAGE_COLS-1)
	    printf("\n");
    }
    if ( idx % USAGE_COLS )
	printf("\n");

    exit(2);
}

error(mesg)
char *mesg;
{
    perror(mesg);
    exit(2);
}

char *font_lib="/usr/lib/vidi/", tmpname[128];

void
set_font(font_idx)
int font_idx;
{
    int fd;
    unsigned short num;
    char *buf;

    if (!filename) {
	strcpy(tmpname, font_lib);
	strcat(tmpname, font_tab[font_idx].name);
	filename=tmpname;
    }

    if ( (fd= open(filename, O_RDONLY)) <0)
	error(filename);

    buf= malloc(font_tab[font_idx].fontsize);
    if (buf == NULL)
	error(VIDI);

    errno=0;
    num= read(fd, buf, font_tab[font_idx].fontsize);
    if (errno)
	error(filename);

    if ( num != font_tab[font_idx].fontsize) {
	puts("Incomplete font file");
	exit(2);
    }

    if (ioctl(0, font_tab[font_idx].set_font, buf) == -1)
	error(VIDI);

    free(buf);
    close(fd);
}

void
get_font(font_idx)
int font_idx;
{
    int fd;
    unsigned short num;
    char *buf;

    if (filename) {
	fd= open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd <0)
	    error(filename);
    }
    else
    {
	fd= 1;
	filename= "<stdout>";
    }

    buf= malloc(font_tab[font_idx].fontsize);
    if (buf == NULL)
	error(VIDI);

    if (ioctl(0, font_tab[font_idx].get_font, buf) == -1) {
	if (fd !=1)
		unlink(filename);
	error(VIDI);
    }

    errno=0;
    num= write(fd, buf, font_tab[font_idx].fontsize);
    if (errno || num!= font_tab[font_idx].fontsize)
	error(filename);

    free(buf);
    close(fd);
}

main(argc, argv)
char	*argv[];
{
    register int idx;

    if (argc == 1)
	usage();

    while( *++argv && (argv[0][0] == '-') && (argv[0][2] == '\0') ) {
	switch(argv[0][1]) {
	    default:
		usage();
	    case 'f':
		if(*++argv == NULL)
		    usage();
		filename= *argv;
		break;
	    case 'd':
		extract++;
		break;
	}
    }

    if (strncmp(*argv, "font", 4) ==0) {
	for(idx= sizeof(font_tab)/sizeof(*font_tab); --idx>=0; ) {
	    if ( strcmp(*argv, font_tab[idx].name) == 0 ) {
		if (extract)
		    get_font(idx);
		else
		    set_font(idx);

		break;
	    }
	}
	if (idx<0) {
	    fprintf(stderr, "unknown font: %s\n", *argv);
	    exit(2);
	}
    }
    else
    {
	for(idx= sizeof(vid_tab)/sizeof(struct vid_tab); --idx>=0; ) {
	    if ( strcmp(*argv, vid_tab[idx].name) == 0 ) {
		if (ioctl(0, vid_tab[idx].mode_code, 0) == -1)
		    error(VIDI);
		break;
	    }
	}
	if (idx<0) {
	    if (*argv)
		fprintf(stderr, "unknown mode: %s\n", *argv);
	    else
		fprintf(stderr, "no font specified\n");
	    exit(2);
	}
    }
    exit(0);
}

