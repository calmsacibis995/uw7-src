/* $XFree86: xc/programs/Xserver/hw/xfree86/xf86config/xf86config.c,v 3.21 1996/01/24 22:03:28 dawes Exp $ */

/*
 * This is a configuration program that will create a base XF86Config
 * file based on menu choices. Its main feature is that clueless users
 * may be less inclined to select crazy sync rates way over monitor spec,
 * by presenting a menu with standard monitor types. Also some people
 * don't read docs unless an executable that they can run tells them to.
 *
 * It assumes a 24-line or bigger text console.
 *
 * Revision history:
 * 25Sep94 Initial version.
 * 27Sep94 Fix hsync range of monitor types to match with best possible mode.
 *         Remove 'const'.
 *         Tweak descriptions.
 * 28Sep94 Fixes from J"org Wunsch:
 *           Don't use gets().
 *           Add mouse device prompt.
 *           Fix lines overrun for 24-line console.
 *         Increase buffer size for probeonly output.
 * 29Sep94 Fix bad bug with old XF86Config preserving during probeonly run.
 *         Add note about vertical refresh in interlaced modes.
 *         Name gets() replacement getstring().
 *         Add warning about binary paths.
 *         Fixes from David Dawes:
 *           Don't use 'ln -sf'.
 *           Omit man path reference in comment.
 *           Generate only a generic 320x200 SVGA section for accel cards.
 *	     Only allow writing to /usr/X11R6/lib/X11 if root, and use
 *	       -xf86config for the -probeonly phase (root only).
 *         Fix bug that forces screen type to accel in some cases.
 * 30Sep94 Continue after clocks probe fails.
 *         Note about programmable clocks.
 *         Rename to 'xf86config'. Not to be confused with XF86Config
 *           or the -xf86config option.
 * 07Oct94 Correct hsync in standard mode timings comments, and include
 *           the proper +/-h/vsync flags.
 * 11Oct94 Skip 'numclocks:' and 'pixel clocks:' lines when probing for
 * 	     clocks.
 * 18Oct94 Add check for existence of /usr/X11R6.
 *	   Add note about ctrl-alt-backspace.
 * 06Nov94 Add comment above standard mode timings in XF86Config.
 * 24Dec94 Add low-resolution modes using doublescan.
 * 29Dec94 Add note in horizontal sync range selection.
 *	   Ask about ClearDTR/RTS option for Mouse Systems mice.
 *	   Ask about writing to /etc/XF86Config.
 *	   Allow link to be set in /var/X11R6/bin.
 *	   Note about X -probeonly crashing.
 *	   Add keyboard Alt binding option for non-ASCII characters.
 *	   Add card database selection.
 *	   Write temporary XF86Config for clock probing in /tmp instead
 *	     of /usr/X11R6/lib/X11.
 *	   Add RAMDAC and Clockchip menu.
 *
 * Possible enhancements:
 * - Add more standard mode timings (also applies to README.Config). Missing
 *   are 1024x768 @ 72 Hz, 1152x900 modes, and 1280x1024 @ ~70 Hz.
 *   I suspect there is a VESA standard for 1024x768 @ 72 Hz with 77 MHz dot
 *   clock, and 1024x768 @ 75 Hz with 78.7 MHz dot clock. New types of
 *   monitors probably work better with VESA 75 Hz timings.
 * - Add option for creation of clear, minimal XF86Config.
 * - The card database doesn't include most of the entries in previous
 *   databases.
 *
 * Send comments to hhanemaa@cs.ruu.nl.
 *
 * Things to keep up-to-date:
 * - Accelerated server names.
 * - Ramdac and Clockchip settings.
 * - The card database.
 *
 */
/* $XConsortium: xf86config.c /main/11 1996/01/26 14:04:59 kaleb $ */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "cards.h"


/*
 * Define the following to 310 to remove references to XFree86 features that
 * have been added since XFree86 3.1 (e.g. DoubleScan modes).
 */
#define XFREE86_VERSION 313

/*
 * This is the filename of the temporary XF86Config file that is written
 * when the program is told to probe clocks (which can only happen for
 * root).
 */
#ifndef __EMX__
#define TEMPORARY_XF86CONFIG_FILENAME "/tmp/XF86Config.tmp"
#else
/* put in root dir, would have to find TMP dir first else */
#define TEMPORARY_XF86CONFIG_FILENAME "/XConfig.tmp"
#endif

/*
 * Define this to have /etc/XF86Config prompted for as the default
 * location to write the XF86Config file to.
 */
#define PREFER_XF86CONFIG_IN_ETC

/*
 * Configuration variables.
 */

#define MAX_CLOCKS_LINES 16

/* (hv) make a number of filenames defines, because I want OS/2 to need just
 * 8.3 names here
 */
#ifdef __EMX__
#define DUMBCONFIG2 "/dconfig.2"
#define DUMBCONFIG3 "/dconfig.3"
#else
#define DUMBCONFIG2 "/tmp/dumbconfig.2"
#define DUMBCONFIG3 "/tmp/dumbconfig.3"
#endif

/* some more vars to make path names in texts more flexible. OS/2 users
 * may be more irritated than Unix users
 */
#ifndef __EMX__
#define TREEROOT "/usr/X11R6"
#define TREEROOTLX "/usr/X11R6/lib/X11"
#define CONFIGNAME "XF86Config"
#else
#define TREEROOT "/XFree86"
#define TREEROOTLX "/XFree86/lib/X11"
#define CONFIGNAME "XConfig"
#endif

int config_mousetype;		/* Mouse. */
int config_emulate3buttons;
int config_chordmiddle;
int config_cleardtrrts;
char *config_pointerdevice;
int config_altmeta;		/* Keyboard. */
int config_monitortype;		/* Monitor. */
char *config_hsyncrange;
char *config_vsyncrange;
char *config_monitoridentifier;
char *config_monitorvendorname;
char *config_monitormodelname;
int config_videomemory;		/* Video card. */
int config_screentype;		/* mono, vga16, svga, accel */
char *config_deviceidentifier;
char *config_devicevendorname;
char *config_deviceboardname;
int config_numberofclockslines;
char *config_clocksline[MAX_CLOCKS_LINES];
char *config_modesline8bpp;
char *config_modesline16bpp;
char *config_modesline32bpp;
int config_virtualx8bpp, config_virtualy8bpp;
int config_virtualx16bpp, config_virtualy16bpp;
int config_virtualx32bpp, config_virtualy32bpp;
char *config_ramdac;
char *config_dacspeed;
char *config_clockchip;

/*
 * These are from the selected card definition. Parameters from the
 * definition are offered during the questioning about the video card.
 */

int card_selected;	/* Card selected from database. */
int card_screentype;
int card_accelserver;


void write_XF86Config();


/*
 * This is the initial intro text that appears when the program is started.
 */

static char *intro_text =
"\n"
"This program will create a basic " CONFIGNAME " file, based on menu selections you\n"
"make.\n"
"\n"
"The " CONFIGNAME " file usually resides in " TREEROOTLX " or /etc. A sample\n"
CONFIGNAME " file is supplied with XFree86; it is configured for a standard\n"
"VGA card and monitor with 640x480 resolution. This program will ask for a\n"
"pathname when it is ready to write the file.\n"
"\n"
"You can either take the sample " CONFIGNAME " as a base and edit it for your\n"
"configuration, or let this program produce a base " CONFIGNAME " file for your\n"
"configuration and fine-tune it. Refer to " TREEROOTLX "/doc/README.Config\n"
"for a detailed overview of the configuration process.\n"
"\n"
"For accelerated servers (including accelerated drivers in the SVGA server),\n"
"there are many chipset and card-specific options and settings. This program\n"
"does not know about these. On some configurations some of these settings must\n"
"be specified. Refer to the server man pages and chipset-specific READMEs.\n"
"\n"
"Before continuing with this program, make sure you know the chipset and\n"
"amount of video memory on your video card. SuperProbe can help with this.\n"
"It is also helpful if you know what server you want to run.\n"
"\n"
;

static char *finalcomment_text =
"File has been written. Take a look at it before running 'startx'. Note that\n"
"the XF86Config file must be in one of the directories searched by the server\n"
"(e.g. " TREEROOTLX ") in order to be used. Within the server press\n"
"ctrl, alt and '+' simultaneously to cycle video resolutions. Pressing ctrl,\n"
"alt and backspace simultaneously immediately exits the server (use if\n"
"the monitor doesn't sync for a particular mode).\n"
"\n"
"For further configuration, refer to " TREEROOTLX "/doc/README.Config.\n"
"\n";


void keypress() {
	printf("Press enter to continue, or ctrl-c to abort.");
	getchar();
	printf("\n");
}

void emptylines() {
	int i;
	for (i = 0; i < 50; i++)
		printf("\n");
}

int answerisyes(s)
	char *s;
{
	if (s[0] == '\'')	/* For fools that type the ' literally. */
		return tolower(s[1]) == 'y';
	return tolower(s[0]) == 'y';
}

/*
 * This is a replacement for gets(). Limit is 80 chars.
 * The 386BSD descendants scream about using gets(), for good reason.
 */

void getstring(s)
	char *s;
{
	char *cp;
	fgets(s, 80, stdin);
	cp = strchr(s, '\n');
	if (cp)
		*cp=0;
}

/*
 * Mouse configuration.
 *
 * (hv) OS/2 (__EMX__) only has an OS supported mouse, so user has no options
 * the server will enable a third button automatically if there is one
 */

static char *mousetype_identifier[9] = {
	"Microsoft",
	"MouseSystems",
	"Busmouse",
	"PS/2",
	"Logitech",
	"MouseMan",
	"MMSeries",
	"MMHitTab",
#ifdef __EMX__
	"OSMOUSE"
#endif
};

#ifndef __EMX__
static char *mouseintro_text =
"First specify a mouse protocol type. Choose one from the following list:\n"
"\n";

static char *mousetype_name[8] = {
	"Microsoft compatible (2-button protocol)",
	"Mouse Systems (3-button protocol)",
	"Bus Mouse",
	"PS/2 Mouse",
	"Logitech Mouse (serial, old type, Logitech protocol)",
	"Logitech MouseMan (Microsoft compatible)",
	"MM Series",	/* XXXX These descriptions should be improved. */
	"MM HitTablet"
};

static char *mousedev_text =
"Now give the full device name that the mouse is connected to, for example\n"
"/dev/tty00. Just pressing enter will use the default, /dev/mouse.\n"
"\n";

static char *mousecomment_text =
"If you have a two-button mouse, it is most likely of type 1, and if you have\n"
"a three-button mouse, it can probably support both protocol 1 and 2. There are\n"
"two main varieties of the latter type: mice with a switch to select the\n"
"protocol, and mice that default to 1 and require a button to be held at\n"
"boot-time to select protocol 2. Some mice can be convinced to do 2 by sending\n"
"a special sequence to the serial port (see the ClearDTR/ClearRTS options).\n"
"\n";

static char *twobuttonmousecomment_text =
"You have selected a two-button mouse protocol. It is recommended that you\n"
"enable Emulate3Buttons.\n";

static char *threebuttonmousecomment_text =
"You have selected a three-button mouse protocol. It is recommended that you\n"
"do not enable Emulate3Buttons, unless the third button doesn't work.\n";

static char *unknownbuttonsmousecomment_text =
"If your mouse has only two buttons, it is recommended that you enable\n"
"Emulate3Buttons.\n";

static char *microsoftmousecomment_text =
"You have selected a Microsoft protocol mouse. If your mouse was made by\n"
"Logitech, you might want to enable ChordMiddle which could cause the\n"
"third button to work.\n";

static char *mousesystemscomment_text =
"You have selected a Mouse Systems protocol mouse. If your mouse is normally\n"
"in Microsoft-compatible mode, enabling the ClearDTR and ClearRTS options\n"
"may cause it to switch to Mouse Systems mode when the server starts.\n";

static char *logitechmousecomment_text =
"You have selected a Logitech protocol mouse. This is only valid for old\n"
"Logitech mice.\n";

static char *mousemancomment_text =
"You have selected a Logitech MouseMan type mouse. You might want to enable\n"
"ChordMiddle which could cause the third button to work.\n";

#endif /* !__EMX__ */

void mouse_configuration() {

#ifndef __EMX__
	int i;
	char s[80];
	printf("%s", mouseintro_text);
	
	for (i = 0; i < 8; i++)
		printf("%2d.  %s\n", i + 1, mousetype_name[i]);

	printf("\n");

	printf("%s", mousecomment_text);
	
	printf("Enter a protocol number: ");
	getstring(s);
	config_mousetype = atoi(s) - 1;

	printf("\n");

	if (config_mousetype == 4) {
		/* Logitech. */
		printf("%s", logitechmousecomment_text);
		printf("\n");
		printf("Please answer the following question with either 'y' or 'n'.\n");
		printf("Are you sure it's really not a Microsoft compatible one? ");
		getstring(s);
		if (!answerisyes(s))
			config_mousetype = 0;
		printf("\n");
	}

	config_chordmiddle = 0;
	if (config_mousetype == 0 || config_mousetype == 5) {
		/* Microsoft or MouseMan. */
		if (config_mousetype == 0)
			printf("%s", microsoftmousecomment_text);
		else
			printf("%s", mousemancomment_text);
		printf("\n");
		printf("Please answer the following question with either 'y' or 'n'.\n");
		printf("Do you want to enable ChordMiddle? ");
		getstring(s);
		if (answerisyes(s))
			config_chordmiddle = 1;
		printf("\n");
	}

	config_cleardtrrts = 0;
	if (config_mousetype == 1) {
		/* Mouse Systems. */
		printf("%s", mousesystemscomment_text);
		printf("\n");
		printf("Please answer the following question with either 'y' or 'n'.\n");
		printf("Do you want to enable ClearDTR and ClearRTS? ");
		getstring(s);
		if (answerisyes(s))
			config_cleardtrrts = 1;
		printf("\n");
	}

	switch (config_mousetype) {
	case 0 : /* Microsoft compatible */
		if (config_chordmiddle)
			printf("%s", threebuttonmousecomment_text);
		else
			printf("%s", twobuttonmousecomment_text);
		break;
	case 1 : /* Mouse Systems. */
		printf("%s", threebuttonmousecomment_text);
		break;
	default :
		printf("%s", unknownbuttonsmousecomment_text);
		break;
	}

	printf("\n");

	printf("Please answer the following question with either 'y' or 'n'.\n");
	printf("Do you want to enable Emulate3Buttons? ");
	getstring(s);
	if (answerisyes(s))
		config_emulate3buttons = 1;
	else
		config_emulate3buttons = 0;
	printf("\n");

	printf("%s", mousedev_text);
	printf("Mouse device: ");
	getstring(s);
	if (strlen(s) == 0)
		config_pointerdevice = "/dev/mouse";
	else {
		config_pointerdevice = malloc(strlen(s) + 1);
		strcpy(config_pointerdevice, s);
       }
       printf("\n");

#else /* __EMX__ */
       	/* set some reasonable defaults for OS/2 */
       	config_mousetype = 8;
	config_chordmiddle = 0;       
	config_cleardtrrts = 0;
	config_emulate3buttons = 0;
	config_pointerdevice = "OS2MOUSE";
#endif /* __EMX__ */
}


/*
 * Keyboard configuration.
 */

/* XXXX Does this make sense? */

static char *keyboardalt_text =
"If you want your keyboard to generate non-ASCII characters in X, because\n"
"you want to be able to enter language-specific characters, you can\n"
"set the left Alt key to Meta, and the right Alt key to ModeShift.\n"
"\n";

void keyboard_configuration() {
	char s[80];
	printf("%s", keyboardalt_text);

	printf("Please answer the following question with either 'y' or 'n'.\n");
	printf("Do you want to enable these bindings for the Alt keys? ");
	getstring(s);
	config_altmeta = 0;
	if (answerisyes(s))
		config_altmeta = 1;
	printf("\n");
}


/*
 * Monitor configuration.
 */

static char *monitorintro_text =
"Now we want to set the specifications of the monitor. The two critical\n"
"parameters are the vertical refresh rate, which is the rate at which the\n"
"the whole screen is refreshed, and most importantly the horizontal sync rate,\n"
"which is the rate at which scanlines are displayed.\n"
"\n"
"The valid range for horizontal sync and vertical sync should be documented\n"
"in the manual of your monitor. If in doubt, check the monitor database\n"
TREEROOTLX "/doc/Monitors to see if your monitor is there.\n"
"\n";

static char *hsyncintro_text =
"You must indicate the horizontal sync range of your monitor. You can either\n"
"select one of the predefined ranges below that correspond to industry-\n"
"standard monitor types, or give a specific range.\n"
"\n"
"It is VERY IMPORTANT that you do not specify a monitor type with a horizontal\n"
"sync range that is beyond the capabilities of your monitor. If in doubt,\n"
"choose a conservative setting.\n"
"\n";

static char *customhsync_text =
"Please enter the horizontal sync range of your monitor, in the format used\n"
"in the table of monitor types above. You can either specify one or more\n"
"continuous ranges (e.g. 15-25, 30-50), or one or more fixed sync frequencies.\n"
"\n";

static char *vsyncintro_text =
"You must indicate the vertical sync range of your monitor. You can either\n"
"select one of the predefined ranges below that correspond to industry-\n"
"standard monitor types, or give a specific range. For interlaced modes,\n"
"the number that counts is the high one (e.g. 87 Hz rather than 43 Hz).\n"
"\n"
" 1  50-70\n"
" 2  50-90\n"
" 3  50-100\n"
" 4  40-150\n"
" 5  Enter your own vertical sync range\n";

static char *monitordescintro_text =
"You must now enter a few identification/description strings, namely an\n"
"identifier, a vendor name, and a model name. Just pressing enter will fill\n"
"in default names.\n"
"\n";

#define NU_MONITORTYPES 10

static char *monitortype_range[NU_MONITORTYPES] = {
	"31.5",
	"31.5 - 35.1",
	"31.5, 35.5",
	"31.5, 35.15, 35.5",
	"31.5 - 37.9",
	"31.5 - 48.5",
	"31.5 - 57.0",
	"31.5 - 64.3",
	"31.5 - 79.0",
	"31.5 - 82.0"
};

static char *monitortype_name[NU_MONITORTYPES] = {
	"Standard VGA, 640x480 @ 60 Hz",
	"Super VGA, 800x600 @ 56 Hz",
	"8514 Compatible, 1024x768 @ 87 Hz interlaced (no 800x600)",
	"Super VGA, 1024x768 @ 87 Hz interlaced, 800x600 @ 56 Hz",
	"Extended Super VGA, 800x600 @ 60 Hz, 640x480 @ 72 Hz",
	"Non-Interlaced SVGA, 1024x768 @ 60 Hz, 800x600 @ 72 Hz",
	"High Frequency SVGA, 1024x768 @ 70 Hz",
	"Monitor that can do 1280x1024 @ 60 Hz",
	"Monitor that can do 1280x1024 @ 74 Hz",
	"Monitor that can do 1280x1024 @ 76 Hz"
};

void monitor_configuration() {
	int i;
	char s[80];
	printf("%s", monitorintro_text);

	keypress();
	emptylines();

	printf("%s", hsyncintro_text);

	printf("    hsync in kHz; monitor type with characteristic modes\n");
	for (i = 0; i < NU_MONITORTYPES; i++)
		printf("%2d  %s; %s\n", i + 1, monitortype_range[i],
			monitortype_name[i]);

	printf("%2d  Enter your own horizontal sync range\n",
		NU_MONITORTYPES + 1);
	printf("\n");

	printf("Enter your choice (1-%d): ", NU_MONITORTYPES + 1);
	getstring(s);
	config_monitortype = atoi(s) - 1;

	printf("\n");

	if (config_monitortype < NU_MONITORTYPES)
		config_hsyncrange = monitortype_range[config_monitortype];
	else {
		/* Custom hsync range option selected. */
		printf("%s", customhsync_text);
		printf("Horizontal sync range: ");
		getstring(s);
		config_hsyncrange = malloc(strlen(s) + 1);
		strcpy(config_hsyncrange, s);
		printf("\n");
	}

	printf("%s", vsyncintro_text);
	printf("\n");

	printf("Enter your choice: ");
	getstring(s);
	printf("\n");
	switch (atoi(s)) {
	case 1 :
		config_vsyncrange = "50-70";
		break;
	case 2 :
		config_vsyncrange = "50-90";
		break;
	case 3 :
		config_vsyncrange = "50-100";
		break;
	case 4 :
		config_vsyncrange = "40-150";
		break;
	case 5 :
		/* Custom vsync range option selected. */
		printf("Vertical sync range: ");
		getstring(s);
		config_vsyncrange = malloc(strlen(s) + 1);
		strcpy(config_vsyncrange, s);
		printf("\n");
		break;
	}
	printf("%s", monitordescintro_text);
	printf("The strings are free-form, spaces are allowed.\n");
	printf("Enter an identifier for your monitor definition: ");
	getstring(s);
	if (strlen(s) == 0)
		config_monitoridentifier = "My Monitor";
	else {
		config_monitoridentifier = malloc(strlen(s) + 1);
		strcpy(config_monitoridentifier, s);
	}
	printf("Enter the vendor name of your monitor: ");
	getstring(s);
	if (strlen(s) == 0)
		config_monitorvendorname = "Unknown";
	else {
		config_monitorvendorname = malloc(strlen(s) + 1);
		strcpy(config_monitorvendorname, s);
	}
	printf("Enter the model name of your monitor: ");
	getstring(s);
	if (strlen(s) == 0)
		config_monitormodelname = "Unknown";
	else {
		config_monitormodelname = malloc(strlen(s) + 1);
		strcpy(config_monitormodelname, s);
	}
}


/*
 * Card database.
 */

static char *cardintro_text =
"Now we must configure video card specific settings. At this point you can\n"
"choose to make a selection out of a database of video card definitions.\n"
"Because there can be variation in Ramdacs and clock generators even\n"
"between cards of the same model, it is not sensible to blindly copy\n"
"the settings (e.g. a Device section). For this reason, after you make a\n"
"selection, you will still be asked about the components of the card, with\n"
"the settings from the chosen database entry presented as a strong hint.\n"
"\n"
"The database entries include information about the chipset, what server to\n"
"run, the Ramdac and ClockChip, and comments that will be included in the\n"
"Device section. However, a lot of definitions only hint about what server\n"
"to run (based on the chipset the card uses) and are untested.\n"
"\n"
"If you can't find your card in the database, there's nothing to worry about.\n"
"You should only choose a database entry that is exactly the same model as\n"
"your card; choosing one that looks similar is just a bad idea (e.g. a\n"
"GemStone Snail 64 may be as different from a GemStone Snail 64+ in terms of\n"
"hardware as can be).\n"
"\n";

static char *cardunsupported_text =
"This card is basically UNSUPPORTED. It may only work as a generic\n"
"VGA-compatible card. If you have an XFree86 version more recent than what\n"
"this card definition was based on, there's a chance that it is now\n"
"supported.\n";

#define NU_ACCELSERVER_IDS 8

static char *accelserver_id[NU_ACCELSERVER_IDS] = {
	"S3", "Mach32", "Mach8", "8514", "P9000", "AGX", "W32", "Mach64"
};

void carddb_configuration() {
	int i;
	char s[80];
	card_selected = -1;
	card_screentype = -1;
	printf("%s", cardintro_text);
	printf("Do you want to look at the card database? ");
	getstring(s);
	printf("\n");
	if (!answerisyes(s))
		return;

	/*
	 * Choose a database entry.
	 */
	if (parse_database()) {
		printf("Couldn't read card database file %s.\n",
			CARD_DATABASE_FILE);
		keypress();
		return;
	}

	i = 0;
	for (;;) {
		int j;
		emptylines();
		for (j = i; j < i + 18 && j <= lastcard; j++)
			printf("%3d  %-50s%s\n", j,
				card[j].name,
				card[j].chipset);
		printf("\n");
		printf("Enter a number to choose the corresponding card definition.\n");
		printf("Press enter for the next page, q to continue configuration.\n");
		printf("\n");
		getstring(s);
		if (s[0] == 'q')
			break;
		if (strlen(s) == 0) {
			i += 18;
			if (i > lastcard)
				i = 0;
			continue;
		}
		card_selected = atoi(s);
		if (card_selected >= 0 && card_selected <= lastcard)
			break;
	}

	/*
	 * Look at the selected card.
	 */
	if (card_selected != -1) {
		if (strcmp(card[card_selected].server, "Mono") == 0)
			card_screentype = 1;
		else
		if (strcmp(card[card_selected].server, "VGA16") == 0)
			card_screentype = 2;
		if (strcmp(card[card_selected].server, "SVGA") == 0)
			card_screentype = 3;
		for (i = 0; i < NU_ACCELSERVER_IDS; i++)
			if (strcmp(card[card_selected].server,
			accelserver_id[i]) == 0) {
				card_screentype = 4;
				card_accelserver = i;
				break;
			}

		printf("\nYour selected card definition:\n\n");
		printf("Identifier: %s\n", card[card_selected].name);
		printf("Chipset:    %s\n", card[card_selected].chipset);
		printf("Server:     XF86_%s\n", card[card_selected].server);
		if (card[card_selected].ramdac != NULL)
			printf("Ramdac:     %s\n", card[card_selected].ramdac);
		if (card[card_selected].dacspeed != NULL)
			printf("DacSpeed:     %s\n", card[card_selected].dacspeed);
		if (card[card_selected].clockchip != NULL)
			printf("Clockchip:  %s\n", card[card_selected].clockchip);
		if (card[card_selected].flags & NOCLOCKPROBE)
			printf("Do NOT probe clocks or use any Clocks line.\n");
		if (card[card_selected].flags & UNSUPPORTED)
			printf("%s", cardunsupported_text);
#if 0	/* Might be confusing. */
		if (strlen(card[card_selected].lines) > 0)
			printf("Device section text:\n%s",
				card[card_selected].lines);
#endif
		printf("\n");
		keypress();
	}
}


/*
 * Screen/video card configuration.
 */

static char *screenintro_text =
"Now you must determine which server to run. Refer to the manpages and other\n"
"documentation. The following servers are available (they may not all be\n"
"installed on your system):\n"
"\n"
" 1  The XF86_Mono server. This a monochrome server that should work on any\n"
"    VGA-compatible card, in 640x480 (more on some SVGA chipsets).\n"
" 2  The XF86_VGA16 server. This is a 16-color VGA server that should work on\n"
"    any VGA-compatible card.\n"
" 3  The XF86_SVGA server. This is a 256 color SVGA server that supports\n"
"    a number of SVGA chipsets. It is accelerated on some Cirrus and WD\n"
"    chipsets; it supports 16/32-bit color on certain Cirrus configurations.\n"
" 4  The accelerated servers. These include XF86_S3, XF86_Mach32, XF86_Mach8,\n"
#if XFREE86_VERSION >= 311
"    XF86_8514, XF86_P9000, XF86_AGX, XF86_W32 and XF86_Mach64.\n"
#else
"    XF86_8514, XF86_P9000, XF86_AGX, and XF86_W32.\n"
#endif
"\n"
"These four server types correspond to the four different \"Screen\" sections in\n"
"XF86Config (vga2, vga16, svga, accel).\n"
"\n";

#ifndef __EMX__
static char *screenlink_text =
"The server to run is selected by changing the symbolic link 'X'. For example,\n"
"'rm /usr/X11R6/bin/X; ln -s /usr/X11R6/bin/XF86_SVGA /usr/X11R6/bin/X' selects\n"
"the SVGA server.\n"
"\n";

static char *varlink_text =
"The directory /var/X11R6/bin exists. On many Linux systems this is the\n"
"preferred location of the symbolic link 'X'. You can select this location\n"
"when setting the symbolic link.\n"
"\n";
#endif /* !__EMX__ */

static char *deviceintro_text =
"Now you must give information about your video card. This will be used for\n"
"the \"Device\" section of your video card in XF86Config.\n"
"\n";

static char *videomemoryintro_text =
"You must indicate how much video memory you have. It is probably a good\n"
"idea to use the same approximate amount as that detected by the server you\n"
"intend to use. If you encounter problems that are due to the used server\n"
"not supporting the amount memory you have (e.g. ATI Mach64 is limited to\n"
"1024K with the SVGA server), specify the maximum amount supported by the\n"
"server.\n"
"\n"
"How much video memory do you have on your video card:\n"
"\n";

static char *screenaccelservers_text =
"Select an accel server:\n"
"\n";

static char *carddescintro_text =
"You must now enter a few identification/description strings, namely an\n"
"identifier, a vendor name, and a model name. Just pressing enter will fill\n"
"in default names (possibly from a card definition).\n"
"\n";

static char *devicevendornamecomment_text =
"You can simply press enter here if you have a generic card, or want to\n"
"describe your card with one string.\n";

static char *devicesettingscomment_text =
"Especially for accelerated servers, Ramdac, Dacspeed and ClockChip settings\n"
"or special options may be required in the Device section.\n"
"\n";

static char *ramdaccomment_text =
"The RAMDAC setting only applies to the S3, AGX, W32 servers, and some \n"
"drivers in the SVGA servers. Some RAMDAC's are auto-detected by the server.\n"
"The detection of a RAMDAC is forced by using a Ramdac \"identifier\" line in\n"
"the Device section. The identifiers are shown at the right of the following\n"
"table of RAMDAC types:\n"
"\n";

#define NU_RAMDACS 23

static char *ramdac_name[NU_RAMDACS] = {
	"AT&T 20C490 (S3 and AGX servers)",
	"AT&T 20C498/21C498/22C498 (S3, autodetected)",
	"AT&T 20C409/20C499 (S3, autodetected)",
	"AT&T 20C505 (S3)",
	"BrookTree BT481 (AGX)",
	"BrookTree BT482 (AGX)",
	"BrookTree BT485/9485 (S3)",
	"Sierra SC15025 (S3, AGX)",
#if XFREE86_VERSION >= 311	
	"S3 GenDAC (86C708) (autodetected)",
	"S3 SDAC (86C716) (autodetected)",
#else
	"S3 GenDAC (86C708)",
	"S3 SDAC (86C716)",
#endif
	"STG-1700 (S3, autodetected)",
	"STG-1703 (S3, autodetected)",
	"TI 3020 (S3, autodetected)",
	"TI 3025 (S3, autodetected)",
	"TI 3026 (S3, autodetected)",
	"IBM RGB 514 (S3, autodetected)",
	"IBM RGB 524 (S3, autodetected)",
	"IBM RGB 525 (S3, autodetected)",
	"IBM RGB 526 (S3)",
	"IBM RGB 528 (S3, autodetected)",
	"ICS5342 (S3, Ark)",
	"ICS5341 (W32)",
	"Normal DAC"
};

static char *ramdac_id[NU_RAMDACS] = {
	"att20c490", "att20c498", "att20c409", "att20c505", "bt481", "bt482",
	"bt485", "sc15025", "s3gendac", "s3_sdac", "stg1700","stg1703",
	"ti3020", "ti3025", "ti3026", "ibm_rgb514", "ibm_rgb524",
	"ibm_rgb525", "ibm_rgb526", "ibm_rgb528", "ics5342", "ics5341",
	"normal"
};

static char *clockchipcomment_text =
"A Clockchip line in the Device section forces the detection of a\n"
"programmable clock device. With a clockchip enabled, any required\n"
"clock can be programmed without requiring probing of clocks or a\n"
"Clocks line. Most cards don't have a programmable clock chip.\n"
"Choose from the following list:\n"
"\n";

#define NU_CLOCKCHIPS 12

static char *clockchip_name[] = {
	"Chrontel 8391",
	"ICD2061A and compatibles (ICS9161A, DCS2824)",
	"ICS2595",
	"ICS5342 (similar to SDAC, but not completely compatible)",
	"ICS5341",
	"S3 GenDAC (86C708) and ICS5300 (autodetected)",
	"S3 SDAC (86C716)",
	"STG 1703 (autodetected)",
	"Sierra SC11412",
	"TI 3025 (autodetected)",
	"TI 3026 (autodetected)",
	"IBM RGB 51x/52x (autodetected)",
};

static char *clockchip_id[] = {
	"ch8391", "icd2061a", "ics2595", "ics5342", "ics5341",
	"s3gendac", "s3_sdac",
	"stg1703", "sc11412", "ti3025", "ti3026", "ibm_rgb5xx",
};

static char *deviceclockscomment_text =
"For most configurations, a Clocks line is useful since it prevents the slow\n"
"and nasty sounding clock probing at server start-up. Probed clocks are\n"
"displayed at server startup, along with other server and hardware\n"
"configuration info. You can save this information in a file by running\n"
"'X -probeonly 2>output_file'. Be warned that clock probing is inherently\n"
"imprecise; some clocks may be slightly too high (varies per run).\n"
"\n";

static char *deviceclocksquestion_text =
"At this point I can run X -probeonly, and try to extract the clock information\n"
"from the output. It is recommended that you do this yourself and add a clocks\n"
"line (note that the list of clocks may be split over multiple Clocks lines) to\n"
"your Device section afterwards. Be aware that a clocks line is not\n"
"appropriate for drivers that have a fixed set of clocks and don't probe by\n"
"default (e.g. Cirrus). Also, for the P9000 server you must simply specify\n"
"clocks line that matches the modes you want to use.  For the S3 server with\n"
"a programmable clock chip you need a 'ClockChip' line and no Clocks line.\n"
"\n"
"You must be root to be able to run X -probeonly now.\n"
"\n";

static char *probeonlywarning_text =
"It is possible that the hardware detection routines in the server will somehow\n"
"cause the system to crash and the screen to remain blank. If this is the\n"
"case, do not choose this option the next time. The server may need a\n"
"Ramdac, ClockChip or special option (e.g. \"nolinear\" for S3) to probe\n"
"and start-up correctly.\n"
"\n";

static char *modesorderintro_text =
"For each depth, a list of modes (resolutions) is defined. The default\n"
"resolution that the server will start-up with will be the first listed\n"
"mode that can be supported by the monitor and card.\n"
"Currently it is set to:\n"
"\n";

static char *modesorder_text2 =
"Note that 16bpp and 32bpp are only supported on a few configurations.\n"
"Modes that cannot be supported due to monitor or clock constraints will\n"
"be automatically skipped by the server.\n"
"\n"
" 1  Change the modes for 8pp (256 colors)\n"
" 2  Change the modes for 16bpp (32K/64K colors)\n"
" 3  Change the modes for 32bpp (24-bit color)\n"
" 4  The modes are OK, continue.\n"
"\n";

static char *modeslist_text =
"Please type the digits corresponding to the modes that you want to select.\n"
"For example, 432 selects \"1024x768\" \"800x600\" \"640x480\", with a\n"
"default mode of 1024x768.\n"
"\n";

#if XFREE86_VERSION >= 311
#define NU_ACCEL_SERVERS 8
#else
#define NU_ACCEL_SERVERS 7
#endif

static char *accelserver_name[NU_ACCEL_SERVERS] = {
	"XF86_S3", "XF86_Mach32", "XF86_Mach8", "XF86_8514", "XF86_P9000",
	"XF86_AGX", "XF86_W32"
#if XFREE86_VERSION >= 311
	,"XF86_Mach64"
#endif
};

static int videomemory[5] = {
	256, 512, 1024, 2048, 4096
};

#if XFREE86_VERSION >= 311
#define NU_MODESTRINGS 8
#else
#define NU_MODESTRINGS 5
#endif

static char *modestring[NU_MODESTRINGS] = {
	"\"640x400\"",
	"\"640x480\"",
	"\"800x600\"",
	"\"1024x768\"",
	"\"1280x1024\"",
#if XFREE86_VERSION >= 311
	"\"320x200\"",
	"\"320x240\"",
	"\"400x300\""
#endif
};

/* (hv) to avoid the UNIXISM to try to open a dir to check for existance */
static int exists_dir(char *name) {
	struct stat sbuf;

	/* is it there ? */
	if (stat(name,&sbuf) == -1)
		return 0;

	/* is there, but is it a dir? */
	return ((sbuf.st_mode & S_IFMT)==S_IFDIR) ? 1 : 0;
}

void screen_configuration() {
	int i, c, varlink;
	int usecardscreentype;
	char s[80];

	printf("%s", screenintro_text);

	if (card_screentype != -1)
		printf(" 5  Choose the server from the card definition, XF86_%s.\n\n",
			card[card_selected].server);

	printf("Which one of these screen types do you intend to run by default (1-%d)? ",
		4 + (card_screentype != -1 ? 1 : 0));
	getstring(s);
	config_screentype = atoi(s);
	usecardscreentype = 0;
	if (config_screentype == 5) {
		config_screentype = card_screentype;	/* From definition. */
		usecardscreentype = 1;
	}
	printf("\n");

#ifndef __EMX__
	printf("%s", screenlink_text);

	varlink = exists_dir("/var/X11R6/bin");
	if (varlink) {
		printf("%s", varlink_text);
	}

	printf("Please answer the following question with either 'y' or 'n'.\n");
	printf("Do you want me to set the symbolic link? ");
	getstring(s);
	printf("\n");
	if (answerisyes(s)) {
		char *servername;
		if (varlink) {
			printf("Do you want to set it in /var/X11R6/bin? ");
			getstring(s);
			printf("\n");
			if (!answerisyes(s))
				varlink = 0;
		}
		if (config_screentype == 4 && usecardscreentype)
			/* Use screen type from card definition. */
			servername = accelserver_name[card_accelserver];
		else
		if (config_screentype == 4) {
			/* Accel server. */
			printf("%s", screenaccelservers_text);
			for (i = 0; i < NU_ACCEL_SERVERS; i++)
				printf("%2d  %s\n", i + 1,
					accelserver_name[i]);
			printf("\n");
			printf("Which accel server: ");
			getstring(s);
			servername = accelserver_name[atoi(s) - 1];
			printf("\n");
		}
		else
			switch (config_screentype) {
			case 1 : servername = "XF86_Mono"; break;
			case 2 : servername = "XF86_VGA16"; break;
			case 3 : servername = "XF86_SVGA"; break;
			}
		if (varlink) {
			system("rm -f /var/X11R6/bin/X");
			sprintf(s, "ln -s /usr/X11R6/bin/%s /var/X11R6/bin/X",
				servername);
		}
		else {
			system("rm -f /usr/X11R6/bin/X");
			sprintf(s, "ln -s /usr/X11R6/bin/%s /usr/X11R6/bin/X",
				servername);
		}
		system(s);
	}
#endif /* !__EMX__ */

	emptylines();

	/*
	 * Configure the "Device" section for the video card.
	 */

	printf("%s", deviceintro_text);

	printf("%s", videomemoryintro_text);

	for (i = 0; i < 5; i++)
		printf("%2d  %dK\n", i + 1, videomemory[i]);
	printf(" 6  Other\n\n");

	printf("Enter your choice: ");
	getstring(s);
	printf("\n");

	c = atoi(s) - 1;
	if (c < 5)
		config_videomemory = videomemory[c];
	else {
		printf("Amount of video memory in Kbytes: ");
		getstring(s);
		config_videomemory = atoi(s);
		printf("\n");
	}

	printf("%s", carddescintro_text);
	if (card_selected != -1)
		printf("Your card definition is %s.\n\n",
			card[card_selected].name);
	printf("The strings are free-form, spaces are allowed.\n");
	printf("Enter an identifier for your video card definition: ");
	getstring(s);
	if (strlen(s) == 0)
		if (card_selected != -1)
			config_deviceidentifier = card[card_selected].name;
		else
			config_deviceidentifier = "My Video Card";
	else {
		config_deviceidentifier = malloc(strlen(s) + 1);
		strcpy(config_deviceidentifier, s);
	}
	printf("%s", devicevendornamecomment_text);
	
	printf("Enter the vendor name of your video card: ");
	getstring(s);
	if (strlen(s) == 0)
		config_devicevendorname = "Unknown";
	else {
		config_devicevendorname = malloc(strlen(s) + 1);
		strcpy(config_devicevendorname, s);
	}
	printf("Enter the model (board) name of your video card: ");
	getstring(s);
	if (strlen(s) == 0)
		config_deviceboardname = "Unknown";
	else {
		config_deviceboardname = malloc(strlen(s) + 1);
                strcpy(config_deviceboardname, s);
	}
	printf("\n");

	emptylines();

	/*
	 * Initialize screen mode variables for svga and accel
	 * to default values.
	 * XXXX Doesn't leave room for off-screen caching in 16/32bpp modes
	 *      for the accelerated servers in some situations.
	 */
	config_modesline8bpp =
	config_modesline16bpp =
	config_modesline32bpp = "\"640x480\"";
	config_virtualx8bpp = config_virtualx16bpp = config_virtualx32bpp =
	config_virtualy8bpp = config_virtualy16bpp = config_virtualy32bpp = 0;
	if (config_videomemory >= 4096) {
		config_virtualx8bpp = 1600;
		config_virtualy8bpp = 1280;
		if (config_screentype == 4) {
			/*
			 * Allow room for font/pixmap cache for accel
			 * servers.
			 */
			config_virtualx16bpp = 1280;
			config_virtualy16bpp = 1024;
		}
		else {
			config_virtualx16bpp = 1600;
			config_virtualy16bpp = 1280;
		}
		if (config_screentype == 4) {
			config_virtualx32bpp = 1024;
			config_virtualy32bpp = 768;
		}
		else {
			config_virtualx32bpp = 1152;
			config_virtualy32bpp = 900;
		}
		/* Add 1600x1280 */
		config_modesline8bpp = "\"640x480\" \"800x600\" \"1024x768\" \"1280x1024\"";
		config_modesline16bpp = "\"640x480\" \"800x600\" \"1024x768\" \"1280x1024\"";
		config_modesline32bpp = "\"640x480\" \"800x600\" \"1024x768\"";
	}
	else
	if (config_videomemory >= 2048) {
		if (config_screentype == 4) {
			/*
			 * Allow room for font/pixmap cache for accel
			 * servers.
			 * Also the mach32 is has a limited width.
			 */
			config_virtualx8bpp = 1280;
			config_virtualy8bpp = 1024;
		}
		else {
			config_virtualx8bpp = 1600;
			config_virtualy8bpp = 1200;
		}
		if (config_screentype == 4) {
			config_virtualx16bpp = 1024;
			config_virtualy16bpp = 768;
		}
		else {
			config_virtualx16bpp = 1152;
			config_virtualy16bpp = 900;
		}
		config_virtualx32bpp = 800;
		config_virtualy32bpp = 600;
		config_modesline8bpp = "\"640x480\" \"800x600\" \"1024x768\" \"1280x1024\"";
		config_modesline16bpp = "\"640x480\" \"800x600\" \"1024x768\"";
		config_modesline32bpp = "\"640x480\" \"800x600\"";
	}
	else
	if (config_videomemory >= 1024) {
		if (config_screentype == 4) {
			/*
			 * Allow room for font/pixmap cache for accel
			 * servers.
			 */
			config_virtualx8bpp = 1024;
			config_virtualy8bpp = 768;
		}
		else {
			config_virtualx8bpp = 1152;
			config_virtualy8bpp = 900;
		}
		config_virtualx16bpp = 800; /* Forget about cache space;  */
		config_virtualy16bpp = 600; /* it's small enough as it is. */
		config_virtualx32bpp = 640;
		config_virtualy32bpp = 400;
		config_modesline8bpp = "\"640x480\" \"800x600\" \"1024x768\"";
		config_modesline16bpp = "\"640x480\" \"800x600\"";
		config_modesline32bpp = "\"640x400\"";
	}
	else
	if (config_videomemory >= 512) {
		config_virtualx8bpp = 800;
		config_virtualy8bpp = 600;
		config_modesline8bpp = "\"640x480\" \"800x600\"";
		config_modesline16bpp = "\"640x400\"";
	}
	else
	if (config_videomemory >= 256) {
		config_modesline8bpp = "640x400";
		config_virtualx8bpp = 640;
		config_virtualy8bpp = 400;
	}
	else {
		printf("Fatal error: Invalid amount of video memory.\n");
		exit(-1);
	}

	/*
	 * Handle the Ramdac/Clockchip setting.
	 */

	printf("%s", devicesettingscomment_text);

	if (config_screentype < 3)
		goto skipramdacselection;

	printf("%s", ramdaccomment_text);

	for (i = 0; i < NU_RAMDACS; i++)
		printf("%2d  %-60s%s\n",
			i + 1, ramdac_name[i], ramdac_id[i]);

	printf("\n");

	if (card_selected != -1)
		if (card[card_selected].ramdac != NULL)
			printf("The card definition has Ramdac \"%s\".\n\n",
				card[card_selected].ramdac);

	printf("Just press enter if you don't want a Ramdac setting.\n");
	printf("What Ramdac setting do you want (1-%d)? ", NU_RAMDACS);

	getstring(s);
	config_ramdac = NULL;
	if (strlen(s) > 0)
		config_ramdac = ramdac_id[atoi(s) - 1];

	printf("\n");
skipramdacselection:

	printf("%s", clockchipcomment_text);

	for (i = 0; i < NU_CLOCKCHIPS; i++)
		printf("%2d  %-60s%s\n",
			i + 1, clockchip_name[i], clockchip_id[i]);

	printf("\n");

	if (card_selected != -1)
		if (card[card_selected].clockchip != NULL)
			printf("The card definition has Clockchip \"%s\"\n\n",
				card[card_selected].clockchip);

	printf("Just press enter if you don't want a Clockchip setting.\n");
	printf("What Clockchip setting do you want (1-%d)? ", NU_CLOCKCHIPS);

	getstring(s);
	config_clockchip = NULL;
	if (strlen(s) > 0)
		config_clockchip = clockchip_id[atoi(s) - 1];

	emptylines();

	/*
	 * Optionally run X -probeonly to figure out the clocks.
	 */

	config_numberofclockslines = 0;

	printf("%s", deviceclockscomment_text);

	printf("%s", deviceclocksquestion_text);

	if (card_selected != -1)
		if (card[card_selected].flags & NOCLOCKPROBE)
			printf("The card definition says to NOT probe clocks.\n");

	if (config_clockchip != NULL) {
		printf("Because you have enabled a Clockchip line, there's no need for clock\n"
			"probing.\n");
		keypress();
		goto skipclockprobing;
	}

	printf("Do you want me to run 'X -probeonly' now? ");
	getstring(s);
	printf("\n");
	if (answerisyes(s)) {
		/*
		 * Write temporary XF86Config and run X -probeonly.
		 * Only allow when root.
		 */
		FILE *f;
		char *buf;
		if (getuid() != 0) {
			printf("Sorry, you must be root to do this.\n\n");
			goto endofprobeonly;
		}
		printf("%s", probeonlywarning_text);
		keypress();
		printf("Running X -probeonly -pn -xf86config "
			TEMPORARY_XF86CONFIG_FILENAME ".\n");
		write_XF86Config(TEMPORARY_XF86CONFIG_FILENAME);
#ifndef __EMX__
		sync();
#endif
		if (system("X -probeonly -pn -xf86config "
		TEMPORARY_XF86CONFIG_FILENAME " 2>" DUMBCONFIG2)) {
			printf("X -probeonly call failed.\n");
			printf("No Clocks line inserted.\n");
			goto clocksprobefailed;
		}
		/* Look for 'clocks:' (case sensitive). */		
		if (system("grep clocks\\: " DUMBCONFIG2 " >" DUMBCONFIG3)) {
			printf("grep failed.\n");
			printf("Cannot find clocks in server output.\n");
			goto clocksprobefailed;
		}
		f = fopen(DUMBCONFIG3, "r");
		buf = malloc(8192);
		/* Parse lines. */
		while (fgets(buf, 8192, f) != NULL) {
			char *clks;
			clks = strstr(buf, "clocks: ") + 8;
			if (clks >= buf + 3 && strcmp(clks - 11, "num") == 0)
				/* Reject lines with 'numclocks:'. */
				continue;
			if (clks >= buf + 8 && strcpy(clks - 14, "pixel ") == 0)
				/* Reject lines with 'pixel clocks:'. */
				continue;
			clks[strlen(clks) - 1] = '\0';	/* Remove '\n'. */
			config_clocksline[config_numberofclockslines] =
				malloc(strlen(clks) + 1);
			strcpy(config_clocksline[config_numberofclockslines],
				clks);
			printf("Clocks %s\n", clks);
			config_numberofclockslines++;
		}
		fclose(f);
clocksprobefailed:
		unlink(DUMBCONFIG3);
		unlink(DUMBCONFIG2);
		unlink(TEMPORARY_XF86CONFIG_FILENAME);
		printf("\n");

endofprobeonly:
		keypress();
	}
skipclockprobing:

	/*
	 * For the mono and vga16 server, no further configuration is
	 * required.
	 */
	if (config_screentype == 1 || config_screentype == 2)
		return;

	/*
	 * Configure the modes order.
	 */
	 for (;;) {
	 	char modes[128];

		emptylines();

		printf("%s", modesorderintro_text);
		printf("%s for 8bpp\n", config_modesline8bpp);
		printf("%s for 16bpp\n", config_modesline16bpp);
		printf("%s for 32bpp\n", config_modesline32bpp);
		printf("\n");
		printf("%s", modesorder_text2);

		printf("Enter your choice: ");
		getstring(s);
		printf("\n");

		c = atoi(s) - 1;
		if (c < 0 || c >= 3)
			break;

		printf("Select modes from the following list:\n\n");

		for (i = 0; i < NU_MODESTRINGS; i++)
			printf("%2d  %s\n", i + 1, modestring[i]);
		printf("\n");

		printf("%s", modeslist_text);

		printf("Which modes? ");
		getstring(s);
		printf("\n");

		modes[0] = '\0';
		for (i = 0; i < strlen(s); i++) {
			if (s[i] < '1' || s[i] > '0' + NU_MODESTRINGS) {
				printf("Invalid mode skipped.\n");
				continue;
			}
			if (i > 0)
				strcat(modes, " ");
			strcat(modes, modestring[s[i] - '1']);
		}
		switch (c) {
		case 0 :
			config_modesline8bpp = malloc(strlen(modes) + 1);
			strcpy(config_modesline8bpp, modes);
			break;
		case 1 :
			config_modesline16bpp = malloc(strlen(modes) + 1);
			strcpy(config_modesline16bpp, modes);
			break;
		case 2 :
			config_modesline32bpp = malloc(strlen(modes) + 1);
			strcpy(config_modesline32bpp, modes);
			break;
		}
	}
}


/*
 * Create the XF86Config file.
 */

static char *XF86Config_firstchunk_text =
"# File generated by xf86config.\n"
"\n"
"#\n"
"# Copyright (c) 1995 by The XFree86 Project, Inc.\n"
"#\n"
"# Permission is hereby granted, free of charge, to any person obtaining a\n"
"# copy of this software and associated documentation files (the \"Software\"),\n"
"# to deal in the Software without restriction, including without limitation\n"
"# the rights to use, copy, modify, merge, publish, distribute, sublicense,\n"
"# and/or sell copies of the Software, and to permit persons to whom the\n"
"# Software is furnished to do so, subject to the following conditions:\n"
"# \n"
"# The above copyright notice and this permission notice shall be included in\n"
"# all copies or substantial portions of the Software.\n"
"# \n"
"# THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
"# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
"# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL\n"
"# THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\n"
"# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF\n"
"# OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
"# SOFTWARE.\n"
"# \n"
"# Except as contained in this notice, the name of the XFree86 Project shall\n"
"# not be used in advertising or otherwise to promote the sale, use or other\n"
"# dealings in this Software without prior written authorization from the\n"
"# XFree86 Project.\n"
"#\n"
"\n"
"# **********************************************************************\n"
"# Refer to the XF86Config(4/5) man page for details about the format of \n"
"# this file.\n"
"# **********************************************************************\n"
"\n"
"# **********************************************************************\n"
"# Files section.  This allows default font and rgb paths to be set\n"
"# **********************************************************************\n"
"\n"
"Section \"Files\"\n"
"\n"
"# The location of the RGB database.  Note, this is the name of the\n"
"# file minus the extension (like \".txt\" or \".db\").  There is normally\n"
"# no need to change the default.\n"
"\n"
"    RgbPath	\"" TREEROOTLX "/rgb\"\n"
"\n"
"# Multiple FontPath entries are allowed (which are concatenated together),\n"
"# as well as specifying multiple comma-separated entries in one FontPath\n"
"# command (or a combination of both methods)\n"
"# \n"
"# If you don't have a floating point coprocessor and emacs, Mosaic or other\n"
"# programs take long to start up, try moving the Type1 and Speedo directory\n"
"# to the end of this list (or comment them out).\n"
"# \n"
"\n"
"    FontPath	\"" TREEROOTLX "/fonts/misc/\"\n"
"    FontPath	\"" TREEROOTLX "/fonts/75dpi/:unscaled\"\n"
"    FontPath	\"" TREEROOTLX "/fonts/100dpi/:unscaled\"\n"
"    FontPath	\"" TREEROOTLX "/fonts/Type1/\"\n"
"    FontPath	\"" TREEROOTLX "/fonts/Speedo/\"\n"
"    FontPath	\"" TREEROOTLX "/fonts/75dpi/\"\n"
"    FontPath	\"" TREEROOTLX "/fonts/100dpi/\"\n"
"\n"
"EndSection\n"
"\n"
"# **********************************************************************\n"
"# Server flags section.\n"
"# **********************************************************************\n"
"\n"
"Section \"ServerFlags\"\n"
"\n"
"# Uncomment this to cause a core dump at the spot where a signal is \n"
"# received.  This may leave the console in an unusable state, but may\n"
"# provide a better stack trace in the core dump to aid in debugging\n"
"\n"
"#    NoTrapSignals\n"
"\n"
"# Uncomment this to disable the <Crtl><Alt><BS> server abort sequence\n"
"# This allows clients to receive this key event.\n"
"\n"
"#    DontZap\n"
"\n"
"# Uncomment this to disable the <Crtl><Alt><KP_+>/<KP_-> mode switching\n"
"# sequences.  This allows clients to receive these key events.\n"
"\n"
"#    DontZoom\n"
"\n"
"EndSection\n"
"\n"
"# **********************************************************************\n"
"# Input devices\n"
"# **********************************************************************\n"
"\n"
"# **********************************************************************\n"
"# Keyboard section\n"
"# **********************************************************************\n"
"\n"
"Section \"Keyboard\"\n"
"\n"
"    Protocol	\"Standard\"\n"
"\n"
"# when using XQUEUE, comment out the above line, and uncomment the\n"
"# following line\n"
"\n"
"#    Protocol	\"Xqueue\"\n"
"\n"
"    AutoRepeat	500 5\n"
"# Let the server do the NumLock processing.  This should only be required\n"
"# when using pre-R6 clients\n"
"#    ServerNumLock\n"
"\n"
"# Specify which keyboard LEDs can be user-controlled (eg, with xset(1))\n"
"#    Xleds      1 2 3\n"
"\n"
"# To set the LeftAlt to Meta, RightAlt key to ModeShift, \n"
"# RightCtl key to Compose, and ScrollLock key to ModeLock:\n"
"\n";

static char *keyboardlastchunk_text =
"#    RightCtl    Compose\n"
"#    ScrollLock  ModeLock\n"
"\n"
"EndSection\n"
"\n"
"\n";

static char *pointersection_text1 = 
"# **********************************************************************\n"
"# Pointer section\n"
"# **********************************************************************\n"
"\n"
"Section \"Pointer\"\n";

static char *pointersection_text2 =
"\n"
"# When using XQUEUE, comment out the above two lines, and uncomment\n"
"# the following line.\n"
"\n"
"#    Protocol	\"Xqueue\"\n"
"\n"
"# Baudrate and SampleRate are only for some Logitech mice\n"
"\n"
"#    BaudRate	9600\n"
"#    SampleRate	150\n"
"\n"
"# Emulate3Buttons is an option for 2-button Microsoft mice\n"
"# Emulate3Timeout is the timeout in milliseconds (default is 50ms)\n"
"\n";

static char *monitorsection_text1 =
"# **********************************************************************\n"
"# Monitor section\n"
"# **********************************************************************\n"
"\n"
"# Any number of monitor sections may be present\n"
"\n"
"Section \"Monitor\"\n"
"\n";

static char *monitorsection_text2 =
"# HorizSync is in kHz unless units are specified.\n"
"# HorizSync may be a comma separated list of discrete values, or a\n"
"# comma separated list of ranges of values.\n"
"# NOTE: THE VALUES HERE ARE EXAMPLES ONLY.  REFER TO YOUR MONITOR\'S\n"
"# USER MANUAL FOR THE CORRECT NUMBERS.\n"
"\n";

static char *monitorsection_text3 =
"#    HorizSync	30-64         # multisync\n"
"#    HorizSync	31.5, 35.2    # multiple fixed sync frequencies\n"
"#    HorizSync	15-25, 30-50  # multiple ranges of sync frequencies\n"
"\n"
"# VertRefresh is in Hz unless units are specified.\n"
"# VertRefresh may be a comma separated list of discrete values, or a\n"
"# comma separated list of ranges of values.\n"
"# NOTE: THE VALUES HERE ARE EXAMPLES ONLY.  REFER TO YOUR MONITOR\'S\n"
"# USER MANUAL FOR THE CORRECT NUMBERS.\n"
"\n";

static char *monitorsection_text4 =
"# Modes can be specified in two formats.  A compact one-line format, or\n"
"# a multi-line format.\n"
"\n"
"# These two are equivalent\n"
"\n"
"#    ModeLine \"1024x768i\" 45 1024 1048 1208 1264 768 776 784 817 Interlace\n"
"\n"
"#    Mode \"1024x768i\"\n"
"#        DotClock	45\n"
"#        HTimings	1024 1048 1208 1264\n"
"#        VTimings	768 776 784 817\n"
"#        Flags		\"Interlace\"\n"
"#    EndMode\n"
"\n";

static char *modelines_text =
"# This is a set of standard mode timings. Modes that are out of monitor spec\n"
"# are automatically deleted by the server (provided the HorizSync and\n"
"# VertRefresh lines are correct), so there's no immediate need to\n"
"# delete mode timings (unless particular mode timings don't work on your\n"
"# monitor). With these modes, the best standard mode that your monitor\n"
"# and video card can support for a given resolution is automatically\n"
"# used.\n"
"\n"
"# 640x400 @ 70 Hz, 31.5 kHz hsync\n"
"Modeline \"640x400\"     25.175 640  664  760  800   400  409  411  450\n"
"# 640x480 @ 60 Hz, 31.5 kHz hsync\n"
"Modeline \"640x480\"     25.175 640  664  760  800   480  491  493  525\n"
"# 800x600 @ 56 Hz, 35.15 kHz hsync\n"
"ModeLine \"800x600\"     36     800  824  896 1024   600  601  603  625\n"
"# 1024x768 @ 87 Hz interlaced, 35.5 kHz hsync\n"
"Modeline \"1024x768\"    44.9  1024 1048 1208 1264   768  776  784  817 Interlace\n"
"\n"
"# 640x480 @ 72 Hz, 36.5 kHz hsync\n"
"Modeline \"640x480\"     31.5   640  680  720  864   480  488  491  521\n"
"# 800x600 @ 60 Hz, 37.8 kHz hsync\n"
"Modeline \"800x600\"     40     800  840  968 1056   600  601  605  628 +hsync +vsync\n"
"\n"
"# 800x600 @ 72 Hz, 48.0 kHz hsync\n"
"Modeline \"800x600\"     50     800  856  976 1040   600  637  643  666 +hsync +vsync\n"
"# 1024x768 @ 60 Hz, 48.4 kHz hsync\n"
"Modeline \"1024x768\"    65    1024 1032 1176 1344   768  771  777  806 -hsync -vsync\n"
"\n"
"# 1024x768 @ 70 Hz, 56.5 kHz hsync\n"
"Modeline \"1024x768\"    75    1024 1048 1184 1328   768  771  777  806 -hsync -vsync\n"
"# 1280x1024 @ 87 Hz interlaced, 51 kHz hsync\n"
"Modeline \"1280x1024\"   80    1280 1296 1512 1568  1024 1025 1037 1165 Interlace\n"
"\n"
"# 1024x768 @ 76 Hz, 62.5 kHz hsync\n"
"Modeline \"1024x768\"    85    1024 1032 1152 1360   768  784  787  823\n"
"# 1280x1024 @ 61 Hz, 64.2 kHz hsync\n"
"Modeline \"1280x1024\"  110    1280 1328 1512 1712  1024 1025 1028 1054\n"
"\n"
"# 1280x1024 @ 74 Hz, 78.85 kHz hsync\n"
"Modeline \"1280x1024\"  135    1280 1312 1456 1712  1024 1027 1030 1064\n"
"\n"
"# 1280x1024 @ 76 Hz, 81.13 kHz hsync\n"
"Modeline \"1280x1024\"  135    1280 1312 1416 1664  1024 1027 1030 1064\n"
"\n"
#if XFREE86_VERSION >= 311
"# Low-res Doublescan modes\n"
"# If your chipset does not support doublescan, you get a 'squashed'\n"
"# resolution like 320x400.\n"
"\n"
"# 320x200 @ 70 Hz, 31.5 kHz hsync, 8:5 aspect ratio\n"
"Modeline \"320x200\"     12.588 320  336  384  400   200  204  205  225 Doublescan\n"
"# 320x240 @ 60 Hz, 31.5 kHz hsync, 4:3 aspect ratio\n"
"Modeline \"320x240\"     12.588 320  336  384  400   240  245  246  262 Doublescan\n"
"# 320x240 @ 72 Hz, 36.5 kHz hsync\n"
"Modeline \"320x240\"     15.750 320  336  384  400   240  244  246  262 Doublescan\n"
"# 400x300 @ 56 Hz, 35.2 kHz hsync, 4:3 aspect ratio\n"
"ModeLine \"400x300\"     18     400  416  448  512   300  301  602  312 Doublescan\n"
"# 400x300 @ 60 Hz, 37.8 kHz hsync\n"
"Modeline \"400x300\"     20     400  416  480  528   300  301  303  314 Doublescan\n"
"# 400x300 @ 72 Hz, 48.0 kHz hsync\n"
"Modeline \"400x300\"     25     400  424  488  520   300  319  322  333 Doublescan\n"
"# 480x300 @ 56 Hz, 35.2 kHz hsync, 8:5 aspect ratio\n"
"ModeLine \"480x300\"     21.656 480  496  536  616   300  301  302  312 Doublescan\n"
"# 480x300 @ 60 Hz, 37.8 kHz hsync\n"
"Modeline \"480x300\"     23.890 480  496  576  632   300  301  303  314 Doublescan\n"
"# 480x300 @ 63 Hz, 39.6 kHz hsync\n"
"Modeline \"480x300\"     25     480  496  576  632   300  301  303  314 Doublescan\n"
"# 480x300 @ 72 Hz, 48.0 kHz hsync\n"
"Modeline \"480x300\"     29.952 480  504  584  624   300  319  322  333 Doublescan\n"
"\n"
#endif
;

static char *devicesection_text =
"# **********************************************************************\n"
"# Graphics device section\n"
"# **********************************************************************\n"
"\n"
"# Any number of graphics device sections may be present\n"
"\n"
"# Standard VGA Device:\n"
"\n"
"Section \"Device\"\n"
"    Identifier	\"Generic VGA\"\n"
"    VendorName	\"Unknown\"\n"
"    BoardName	\"Unknown\"\n"
"    Chipset	\"generic\"\n"
"\n"
"#    VideoRam	256\n"
"\n"
"#    Clocks	25.2 28.3\n"
"\n"
"EndSection\n"
"\n"
"# Sample Device for accelerated server:\n"
"\n"
"# Section \"Device\"\n"
"#    Identifier	\"Actix GE32+ 2MB\"\n"
"#    VendorName	\"Actix\"\n"
"#    BoardName	\"GE32+\"\n"
"#    Ramdac	\"ATT20C490\"\n"
"#    Dacspeed	110\n"
"#    Option	\"dac_8_bit\"\n"
"#    Clocks	 25.0  28.0  40.0   0.0  50.0  77.0  36.0  45.0\n"
"#    Clocks	130.0 120.0  80.0  31.0 110.0  65.0  75.0  94.0\n"
"# EndSection\n"
"\n"
"# Sample Device for Hercules mono card:\n"
"\n"
"# Section \"Device\"\n"
"#    Identifier \"Hercules mono\"\n"
"# EndSection\n"
"\n"
"# Device configured by xf86config:\n"
"\n";

static char *screensection_text1 =
"# **********************************************************************\n"
"# Screen sections\n"
"# **********************************************************************\n"
"\n";


void write_XF86Config(filename)
	char *filename;
{
	FILE *f;

	/*
	 * Write the file.
	 */

	f = fopen(filename, "w");
	if (f == NULL) {
		printf("Failed to open filename for writing.\n");
#ifndef __EMX__
		if (getuid() != 0)
			printf("Maybe you need to be root to write to the specified directory?\n");
#endif
		exit(-1);
	}

	fprintf(f, "%s", XF86Config_firstchunk_text);

	/*
	 * Write keyboard section.
	 */
	if (config_altmeta) {
		fprintf(f, "    LeftAlt     Meta\n");
		fprintf(f, "    RightAlt    ModeShift\n");
	}
	else {
		fprintf(f, "#    LeftAlt     Meta\n");
		fprintf(f, "#    RightAlt    ModeShift\n");
	}
	fprintf(f, "%s", keyboardlastchunk_text);

	/*
	 * Write pointer section.
	 */
	fprintf(f, "%s", pointersection_text1);
	fprintf(f, "    Protocol    \"%s\"\n",
		mousetype_identifier[config_mousetype]);
	fprintf(f, "    Device      \"%s\"\n", config_pointerdevice);
	fprintf(f, "%s", pointersection_text2);
	if (!config_emulate3buttons)
		fprintf(f, "#");
	fprintf(f, "    Emulate3Buttons\n");
	if (!config_emulate3buttons)
		fprintf(f, "#");
	fprintf(f, "    Emulate3Timeout    50\n\n");
	fprintf(f, "# ChordMiddle is an option for some 3-button Logitech mice\n\n");
	if (!config_chordmiddle)
		fprintf(f, "#");
	fprintf(f, "    ChordMiddle\n\n");
	if (config_cleardtrrts) {
		fprintf(f, "    ClearDTR\n");
		fprintf(f, "    ClearRTS\n\n");
	}
	fprintf(f, "EndSection\n\n\n");

	/*
	 * Write monitor section.
	 */
	fprintf(f, "%s", monitorsection_text1);
	fprintf(f, "    Identifier  \"%s\"\n", config_monitoridentifier);
	fprintf(f, "    VendorName  \"%s\"\n", config_monitorvendorname);
	fprintf(f, "    ModelName   \"%s\"\n", config_monitormodelname);
	fprintf(f, "\n");
	fprintf(f, "%s", monitorsection_text2);
	fprintf(f, "    HorizSync   %s\n", config_hsyncrange);
	fprintf(f, "\n");
	fprintf(f, "%s", monitorsection_text3);
	fprintf(f, "    VertRefresh %s\n", config_vsyncrange);
	fprintf(f, "\n");
	fprintf(f, "%s", monitorsection_text4);
	fprintf(f, "%s", modelines_text);
	fprintf(f, "EndSection\n\n\n");

	/*
	 * Write Device section.
	 */

	fprintf(f, "%s", devicesection_text);
	fprintf(f, "Section \"Device\"\n");
	fprintf(f, "    Identifier  \"%s\"\n", config_deviceidentifier);
	fprintf(f, "    VendorName  \"%s\"\n", config_devicevendorname);
	fprintf(f, "    BoardName   \"%s\"\n", config_deviceboardname);
	/* Rely on server to detect video memory. */
	fprintf(f, "    #VideoRam    %d\n", config_videomemory);
	if (card_selected != -1)
		/* Add comment lines from card definition. */
		fprintf(f, card[card_selected].lines);
	if (config_ramdac != NULL)
		fprintf(f, "    Ramdac      \"%s\"\n", config_ramdac);
	if (card_selected != -1)
		if (card[card_selected].dacspeed != NULL)
			fprintf(f, "    Dacspeed    %s\n",
				card[card_selected].dacspeed);
	if (config_clockchip != NULL)
		fprintf(f, "    Clockchip   \"%s\"\n", config_clockchip);
	else
	if (config_numberofclockslines == 0)
		fprintf(f, "    # Insert Clocks lines here if appropriate\n");
	else {
		int i;
		for (i = 0; i < config_numberofclockslines; i++)
			fprintf(f, "    Clocks %s\n", config_clocksline[i]);
	}
	fprintf(f, "EndSection\n\n\n");	

	/*
	 * Write Screen sections.
	 */

	fprintf(f, "%s", screensection_text1);

	/*
	 * SVGA screen section.
	 */
	if (config_screentype == 3)
		fprintf(f, 
			"# The Colour SVGA server\n"
			"\n"
			"Section \"Screen\"\n"
			"    Driver      \"svga\"\n"
			"    # Use Device \"Generic VGA\" for Standard VGA 320x200x256\n"
			"    #Device      \"Generic VGA\"\n"
			"    Device      \"%s\"\n"
			"    Monitor     \"%s\"\n"
			"    Subsection \"Display\"\n"
			"        Depth       8\n"
			"        # Omit the Modes line for the \"Generic VGA\" device\n"
			"        Modes       %s\n"
			"        ViewPort    0 0\n"
			"        # Use Virtual 320 200 for Generic VGA\n"
			"        Virtual     %d %d\n"
			"    EndSubsection\n"
			"    Subsection \"Display\"\n"
			"        Depth       16\n"
			"        Modes       %s\n"
			"        ViewPort    0 0\n"
			"        Virtual     %d %d\n"
			"    EndSubsection\n"
			"    Subsection \"Display\"\n"
			"        Depth       32\n"
			"        Modes       %s\n"
			"        ViewPort    0 0\n"
			"        Virtual     %d %d\n"
			"    EndSubsection\n"
			"EndSection\n"
			"\n",
			config_deviceidentifier,
			config_monitoridentifier,
			config_modesline8bpp,
			config_virtualx8bpp, config_virtualy8bpp,
			config_modesline16bpp,
			config_virtualx16bpp, config_virtualy16bpp,
			config_modesline32bpp,
 			config_virtualx32bpp, config_virtualy32bpp
		);
	else
		/*
		 * If the default server is not the SVGA server, generate
		 * an SVGA server screen for just generic 320x200.
		 */
		fprintf(f, 
			"# The Colour SVGA server\n"
			"\n"
			"Section \"Screen\"\n"
			"    Driver      \"svga\"\n"
			"    Device      \"Generic VGA\"\n"
			"    #Device      \"%s\"\n"
			"    Monitor     \"%s\"\n"
			"    Subsection \"Display\"\n"
			"        Depth       8\n"
			"        #Modes       %s\n"
			"        ViewPort    0 0\n"
			"        Virtual     320 200\n"
			"        #Virtual     %d %d\n"
			"    EndSubsection\n"
			"EndSection\n"
			"\n",
			config_deviceidentifier,
			config_monitoridentifier,
			config_modesline8bpp,
			config_virtualx8bpp, config_virtualy8bpp
		);

	/*
	 * VGA16 screen section.
	 */
	fprintf(f, 
		"# The 16-color VGA server\n"
		"\n"
		"Section \"Screen\"\n"
		"    Driver      \"vga16\"\n"
		"    Device      \"%s\"\n"
		"    Monitor     \"%s\"\n"
		"    Subsection \"Display\"\n"
		/*
		 * Depend on 800x600 to be deleted if not available due to
		 * dot clock or monitor constraints.
		 */
		"        Modes       \"640x480\" \"800x600\"\n"
		"        ViewPort    0 0\n"
		"        Virtual     800 600\n"
		"    EndSubsection\n"
		"EndSection\n"
		"\n",
		/*
		 * If mono or vga16 is configured as the default server,
		 * use the configured video card device instead of the
		 * generic VGA device.
		 */
		(config_screentype == 1 || config_screentype == 2) ?
			config_deviceidentifier :
			"Generic VGA",
		config_monitoridentifier
	);

	/*
	 * VGA2 screen section.
	 * This is almost identical to the VGA16 section.
	 */
	fprintf(f, 
		"# The Mono server\n"
		"\n"
		"Section \"Screen\"\n"
		"    Driver      \"vga2\"\n"
		"    Device      \"%s\"\n"
		"    Monitor     \"%s\"\n"
		"    Subsection \"Display\"\n"
		/*
		 * Depend on 800x600 to be deleted if not available due to
		 * dot clock or monitor constraints.
		 */
		"        Modes       \"640x480\" \"800x600\"\n"
		"        ViewPort    0 0\n"
		"        Virtual     800 600\n"
		"    EndSubsection\n"
		"EndSection\n"
		"\n",
		/*
		 * If mono or vga16 is configured as the default server,
		 * use the configured video card device instead of the
		 * generic VGA device.
		 */
		(config_screentype == 1 || config_screentype == 2) ?
			config_deviceidentifier :
			"Generic VGA",
		config_monitoridentifier
	);

	/*
	 * The Accel section.
	 */
	fprintf(f, 
#if XFREE86_VERSION >= 311
		"# The accelerated servers (S3, Mach32, Mach8, 8514, P9000, AGX, W32, Mach64)\n"
#else
		"# The accelerated servers (S3, Mach32, Mach8, 8514, P9000, AGX, W32)\n"
#endif
		"\n"
		"Section \"Screen\"\n"
		"    Driver      \"accel\"\n"
		"    Device      \"%s\"\n"
		"    Monitor     \"%s\"\n"
		"    Subsection \"Display\"\n"
		"        Depth       8\n"
		"        Modes       %s\n"
		"        ViewPort    0 0\n"
		"        Virtual     %d %d\n"
		"    EndSubsection\n"
		"    Subsection \"Display\"\n"
		"        Depth       16\n"
		"        Modes       %s\n"
		"        ViewPort    0 0\n"
		"        Virtual     %d %d\n"
		"    EndSubsection\n"
		"    Subsection \"Display\"\n"
		"        Depth       32\n"
		"        Modes       %s\n"
		"        ViewPort    0 0\n"
		"        Virtual     %d %d\n"
		"    EndSubsection\n"
		"EndSection\n"
		"\n",
		config_deviceidentifier,
		config_monitoridentifier,
		config_modesline8bpp,
		config_virtualx8bpp, config_virtualy8bpp,
		config_modesline16bpp,
		config_virtualx16bpp, config_virtualy16bpp,
		config_modesline32bpp,
		config_virtualx32bpp, config_virtualy32bpp
	);

	fclose(f);
}


/*
 * Ask where to write XF86Config to. Returns filename.
 */

char *ask_XF86Config_location() {
	char s[80];
	char *filename;

	printf(
"I am going to write the XF86Config file now. Make sure you don't accidently\n"
"overwrite a previously configured one.\n\n");

#ifndef __EMX__
	if (getuid() == 0) {
#ifdef PREFER_XF86CONFIG_IN_ETC
		printf("Shall I write it to /etc/XF86Config? ");
		getstring(s);
		printf("\n");
		if (answerisyes(s))
			return "/etc/XF86Config";
#endif

		printf("Please answer the following question with either 'y' or 'n'.\n");
		printf("Shall I write it to the default location, /usr/X11R6/lib/X11/XF86Config? ");
		getstring(s);
		printf("\n");
		if (answerisyes(s))
			return "/usr/X11R6/lib/X11/XF86Config";

#ifndef PREFER_XF86CONFIG_IN_ETC
		printf("Shall I write it to /etc/XF86Config? ");
		getstring(s);
		printf("\n");
		if (answerisyes(s))
			return "/etc/XF86Config";
#endif
#else /* __EMX__ */
	{
		printf("Please answer the following question with either 'y' or 'n'.\n");
		printf("Shall I write it to the default location, drive:/XFree86/lib/X11/XConfig? ");
		getstring(s);
		printf("\n");
		if (answerisyes(s)) {
			static char pn[80];
			char *root = getenv("X11ROOT");
			if (!root) root = "";
			sprintf(pn,"%s/XFree86/lib/X11/XConfig",root);
			return pn;
		}
#endif /* __EMX__ */
	}

	printf("Do you want it written to the current directory as 'XF86Config'? ");
	getstring(s);
	printf("\n");
	if (answerisyes(s))
#ifndef __EMX__
		return "XF86Config";
#else
		return "XConfig";
#endif

	printf("Please give a filename to write to: ");
	getstring(s);
	printf("\n");
	filename = malloc(strlen(s) + 1);
	strcpy(filename, s);
	return filename;
}


/*
 * Check if an earlier version of XFree86 is installed; warn about proper
 * search path order in that case.
 */

static char *notinstalled_text =
"The directory " TREEROOT " does not exist. This probably means that you have\n"
"not yet installed an X11R6-based version of XFree86. Please install\n"
"XFree86 3.1+ before running this program, following the instructions in\n"
"the INSTALL or README that comes with the XFree86 distribution for your OS.\n"
"For a minimal installation it is sufficient to only install base binaries,\n"
"libraries, configuration files and a server that you want to use.\n"
"\n";

#ifndef __EMX__
static char *oldxfree86_text =
"The directory '/usr/X386/bin' exists. You probably have an old version of\n"
"XFree86 installed (XFree86 3.1 installs in '" TREEROOT "' instead of\n"
"'/usr/X386').\n"
"\n"
"It is important that the directory '" TREEROOT "' is present in your\n"
"search path, *before* any occurrence of '/usr/X386/bin'. If you have installed\n"
"X program binaries that are not in the base XFree86 distribution in\n"
"'/usr/X386/bin', you can keep the directory in your path as long as it is\n"
"after '" TREEROOT "'.\n"
"\n";

static char *pathnote_text =	
"Note that the X binary directory in your path may be a symbolic link.\n"
"In that case you could modify the symbolic link to point to the new binaries.\n"
"Example: 'rm -f /usr/bin/X11; ln -s /usr/X11R6/bin /usr/bin/X11', if the\n"
"link is '/usr/bin/X11'.\n"
"\n"
"Make sure the path is OK before continuing.\n";
#endif

void path_check() {
	char s[80];
	int ok;

	ok = exists_dir(TREEROOT);
	if (!ok) {
		printf("%s", notinstalled_text);
		printf("Do you want to continue? ");
		getstring(s);
		if (!answerisyes(s))
			exit(-1);
		printf("\n");
	}

#ifndef __EMX__
	ok = exists_dir("/usr/X386/bin");
	if (!ok)
		return;

	printf("%s", oldxfree86_text);
	printf("Your PATH is currently set as follows:\n%s\n\n",
		getenv("PATH"));
	printf("%s", pathnote_text);
	keypress();
#endif
}


/*
 * Program entry point.
 */

void main() {
	emptylines();

	printf("%s", intro_text);

	keypress();
	emptylines();

	path_check();

	emptylines();

	mouse_configuration();

	emptylines();

	keyboard_configuration();

	emptylines();

	monitor_configuration();

	emptylines();

	carddb_configuration();

	emptylines();

 	screen_configuration();

	emptylines();

	write_XF86Config(ask_XF86Config_location());

	printf("%s", finalcomment_text);

	exit(0);
}
