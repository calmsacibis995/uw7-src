/*
 *	@(#) scoConfig.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1991.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/*
 * scoConfig.c
 *	Functions to open and read the server configuration file.
 */

/*
 * Modification History
 *
 * S000, 28-Mar-91, staceyc@sco.com
 * 	created
 * S001, 10-Apr-91, staceyc@sco.com
 * 	added server generation check
 * S002, 13-Apr-91, mikep@sco.com
 * 	scoGetKeyboardMappings needs to happen everytime, but LoadConfig
 *	only need to happen once.
 * S003, 14-Aug-91, mikep@sco.com
 *	fix memory leak in scoGetKeyboardMappings.  Use a static struct
 *	instead of malloc.
 * S004, 14-Aug-91, mikep@sco.com
 *	set scoScanTranslate and scoNumScanTranslate to defaults.
 * S005, 25-Nov-91, mikep@sco.com
 *      zero out memory before writting modifier map to it.
 * S006, 27-Feb-92, mikep@sco.com
 * 	Define DEFAULTCONFIGURATIONFILEPATH here.  It ain't gonna change.
 * S007, 14-Sep-93, mikep@sco.com
 *	check for zero size structs before trying to write to them
 * S008, Sat Sep 14 04:49:25 GMT 1996, kylec
 *	Use LIBDIR for DEFAULTCONFIGURATIONFILEPATH
 * S009, Tue Sep 24 13:25:02 PDT 1996, kylec
 * 	Get configRec data out of xsconfig/config.h.  It's no
 * 	longer in scoIo.h.
 * S010, Mon Oct  7 08:31:00 PDT 1996, kylec@sco.com
 *	Add support for XKEYBOARD extension configuration data
 *	to be read from .Xsco.cfg.
 * S011, Thu Apr  3 15:42:03 PST 1997, kylec@sco.com
 * 	Add i18n support
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "Xproto.h"
#include "sco.h"
#include "os.h"
#include "site.h"
#include "input.h"
#include "scoIo.h"
#include "xsconfig/config.h"

#include "xsrv_msgcat.h"	/* S011 */

#ifdef XKB
#include <X11/extensions/XKB.h>
#include <X11/extensions/XKBstr.h>
#endif


#if !defined(LIBDIR)
#define LIBDIR "/usr/lib/X11"   /* S008 */
#endif

#define DEFAULTCONFIGURATIONFILEPATH LIBDIR /* S008 */
#define KEY_CTRL_SIZE 0x180

extern void scoSetKeyboardDefaults();

ScanTranslation *scoScanTranslate = NULL;
int scoNumScanTranslate = 0;

static char ConfigFileName[] = ".Xsco.cfg";
static ConfigRec ConfigHeader;
static unsigned char *PointerMap;
static int NumConfigColors;
static PixelValue *ConfigColors;
static unsigned char *ConfigKeyCtrl;
static KeySym *scoKeyMap = 0;
static CARD8 *scoModMap = 0;

#ifdef XKB
static XkbComponentNamesRec scoXkbNames = {0, 0, 0, 0, 0, 0};
#endif /* XKB */

static FILE *OpenConfigFile()

{
	FILE *fp;
	char *home = 0, *name = 0;
	int len;

	home = getenv("HOME");
	if (home)
	{
		len = strlen(home) + sizeof ConfigFileName + 2;
		if (! (name = (char *)Xalloc(len)))
		{
			ErrorF(MSGSCO(XSCO_1,"Xalloc failed %s %d\n"), __FILE__, __LINE__);
			return (FILE *)0;
		}
		(void)sprintf(name, "%s/%s", home, ConfigFileName);
		if (fp = fopen(name, "r"))
			return fp;
	}
	Xfree((void*)name);
	len = sizeof DEFAULTCONFIGURATIONFILEPATH + sizeof ConfigFileName + 2;
	if (! (name = (char *)Xalloc(len)))
	{
		ErrorF(MSGSCO(XSCO_1,"Xalloc failed %s %d\n"), __FILE__, __LINE__);
		return (FILE *)0;
	}
	(void)sprintf(name, "%s/%s", DEFAULTCONFIGURATIONFILEPATH,
	    ConfigFileName);
	if (! (fp = fopen(name, "r")))
	{
		ErrorF(MSGSCO(XSCO_2,"Cannot open %s\n"), name);
		Xfree((void*)name);
		return (FILE *)0;
	}
	Xfree((void*)name);
	return fp;
}

static void SetConfigurationDefaults()

{
	scoSetKeyboardDefaults();
}

void scoLoadConfiguration()

{
    FILE *fp;

    if (serverGeneration > 1)   /* S001, S002 */
        return;

    if (! (fp = OpenConfigFile()))
    {
        fclose(fp);
        SetConfigurationDefaults();
        return;
    }

    if ((fread((char *)&ConfigHeader, 1, sizeof(ConfigHeader), fp) !=
         sizeof(ConfigHeader)) || ConfigHeader.headerSize !=
        sizeof(ConfigHeader) || ConfigHeader.keyctrlSize != KEY_CTRL_SIZE)
    {
        fclose(fp);
        SetConfigurationDefaults();
        ErrorF(MSGSCO(XSCO_3,"Warning: invalid server configuration file.\n"));
        return;
    }

    if (ConfigHeader.buttonSize) /* S007 */
    {
        PointerMap = (CARD8 *)Xalloc(ConfigHeader.buttonSize);
        fseek(fp, (long)ConfigHeader.buttonOffset, 0);
        fread(PointerMap, 1, ConfigHeader.buttonSize, fp);
    }


    scoModMap = (CARD8 *)Xalloc(MAP_LENGTH);
    bzero(scoModMap, MAP_LENGTH); /* S006 */
    fseek(fp, (long)ConfigHeader.modmapOffset, 0);
    if (ConfigHeader.minScan < MIN_KEYCODE)
        fread(scoModMap+(MIN_KEYCODE-ConfigHeader.minScan), 1, ConfigHeader.modmapSize, fp);
    else
        fread(scoModMap, 1, ConfigHeader.modmapSize, fp);

    scoKeyMap = (KeySym *)Xalloc(ConfigHeader.keymapSize);
    fseek(fp, (long)ConfigHeader.keymapOffset, 0);
    fread((char *)scoKeyMap, 1, ConfigHeader.keymapSize, fp);

    if (ConfigHeader.translateSize)
    {
        scoScanTranslate =
            (ScanTranslation *)Xalloc(ConfigHeader.translateSize);
        fseek(fp, (long)ConfigHeader.translateOffset, 0);
        fread((char *)scoScanTranslate, 1, ConfigHeader.translateSize,
              fp);
        scoNumScanTranslate = ConfigHeader.translateSize /
            sizeof(ScanTranslation);
    }

    NumConfigColors = ConfigHeader.pixelSize / sizeof(PixelValue);
    if (NumConfigColors)
    {
        ConfigColors = (PixelValue *)Xalloc(ConfigHeader.pixelSize);
        fseek(fp, (long)ConfigHeader.pixelOffset, 0);
        fread((char *)ConfigColors, 1, ConfigHeader.pixelSize, fp);
    }

    if (ConfigHeader.keyctrlSize)
    {
        ConfigKeyCtrl =
            (unsigned char *)Xalloc(ConfigHeader.keyctrlSize);
        fseek(fp, (long)ConfigHeader.keyctrlOffset, 0);
        fread(ConfigKeyCtrl, 1, ConfigHeader.keyctrlSize, fp);
    }

#ifdef XKB
    if (ConfigHeader.xkbNamesSize)
    {
        XkbComponentNamesPtr xkbConfig;
        char *base;

        xkbConfig = (XkbComponentNamesPtr)Xalloc(ConfigHeader.xkbNamesSize);
        fseek(fp, (long)ConfigHeader.xkbNamesOffset, 0);
        fread((char*)xkbConfig, 1, ConfigHeader.xkbNamesSize, fp);

        base = (char*)xkbConfig;
        memset((void*)&scoXkbNames, 0, sizeof(XkbComponentNamesRec));

        if (xkbConfig->keymap)
            scoXkbNames.keymap = base + (int)xkbConfig->keymap;
        if (xkbConfig->keycodes)        
            scoXkbNames.keycodes = base + (int)xkbConfig->keycodes;
        if (xkbConfig->types)
            scoXkbNames.types = base + (int)xkbConfig->types;
        if (xkbConfig->compat)
            scoXkbNames.compat = base + (int)xkbConfig->compat;
        if (xkbConfig->symbols)
            scoXkbNames.symbols = base + (int)xkbConfig->symbols;
        if (xkbConfig->geometry)
            scoXkbNames.geometry = base + (int)xkbConfig->geometry;

#ifdef DEBUG
        ErrorF("XKB config: \n"
               "\tkeymap=%s\n"
               "\tkeycodes=%s\n"
               "\ttypes=%s\n"
               "\tcompat=%s\n"
               "\tsymbols=%s\n"
               "\tgeometry=%s\n",
               scoXkbNames.keymap,
               scoXkbNames.keycodes,
               scoXkbNames.types,
               scoXkbNames.compat,
               scoXkbNames.symbols,
               scoXkbNames.geometry);
#endif

    }
#endif /* XKB */

    fclose(fp);
}

void
scoGetKeyboardMappings(pKeySyms,
                       pModMap,
#ifdef XKB
                       pXkbNames,
#endif
                       defKeySysmRec,
                       defModMap,
                       defScanTranslate,
                       defNumScanTranslate)

    KeySymsRec **pKeySyms;
    CARD8 **pModMap;
    KeySymsRec *defKeySysmRec;
    CARD8 *defModMap;
    ScanTranslation *defScanTranslate;
    int defNumScanTranslate;
#ifdef XKB
    XkbComponentNamesPtr *pXkbNames;
#endif /* XKB */

{
    int i;
    static KeySymsRec keySyms;  /* S003 */

    if (scoModMap && scoKeyMap)
    {
        *pModMap = scoModMap;
        *pKeySyms = &keySyms;   /* S003 */
        keySyms.minKeyCode = ConfigHeader.minScan;
        keySyms.maxKeyCode = ConfigHeader.maxScan;
        keySyms.mapWidth = ConfigHeader.keymapWidth;
        keySyms.map = scoKeyMap;
    }
    else
    {
        *pModMap = defModMap;
        *pKeySyms = defKeySysmRec;
    }

    if(scoScanTranslate == NULL) /* S004 vvv */
    {
        scoScanTranslate = defScanTranslate;
        scoNumScanTranslate = defNumScanTranslate;
    } /* S004 ^^^ */

#ifdef XKB
    *pXkbNames = &scoXkbNames;
#endif /* XKB */
}
