/*
 *	@(#) xsconfig.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1991.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Thu Nov 21 12:25:21 PST 1991	mikep@sco.com
 *	-  Change Xsight to Xsco.
 * 	S001	Tue Sep 24 13:51:26 PDT 1996	kylec@sco.com
 *	- Use MIN_KEYCODE to calculate the scancode bias.
 *	S002	Fri Oct  4 14:58:31 PDT 1996	kylec@sco.com
 *	- Add support for XKEYBOARD extension configuration. (#ifdef XKB)
 *      S003	Fri Oct 11 12:06:42 PDT 1996,   kylec@sco.clom
 *	- Remove scancode bias calculations.  Bias calculations
 *	  should be handeled by the X server.
 *
 */
#if defined(SCCS_ID)
static char Sccs_Id[] = 
	 "@(#) xsconfig.c 11.1 97/10/22"
#endif

/*
   (c) Copyright 1989 by Locus Computing Corporation.  ALL RIGHTS RESERVED.

   This material contains valuable proprietary products and trade secrets
   of Locus Computing Corporation, embodying substantial creative efforts
   and confidential information, ideas and expressions.  No part of this
   material may be used, reproduced or transmitted in any form or by any
   means, electronic, mechanical, or otherwise, including photocopying or
   recording, or in connection with any information storage or retrieval
   system without permission in writing from Locus Computing Corporation.
*/

/*
   xsconfig.c - Xsight configuration compiler
*/


#include <stdio.h>
#include <string.h>
#include <varargs.h>
#include <errno.h>
#include <nl_types.h>

#include <X11/X.h>

#include "config.h"
#include "xsconfig.h"
#include "lexer.h"
#include "symtab.h"
#include "alloc.h"
#include "xserror.h"
#include "sco.h"

#ifdef XKB
#include <X11/extensions/XKBstr.h>
#endif

char *Strerror();
void strlwr();

#if !defined(LIBDIR)
#define LIBDIR "/usr/X11R6.1/lib/X11"
#endif

/*
   Data for current file
*/
int eof = 0;
FILE *curFile = NULL;
char *curFileName = NULL;
int lineno = 0;
   
/*
   Configuration data
*/
ConfigRec config = {0};
unsigned char *buttonMap = NULL;
unsigned char *modifierMap = NULL;
KeySym *keysymMap = NULL;
unsigned char *keyCtrlRec = NULL;

#ifdef XKB
char *xkbNames = NULL;
int xkbNamesSize = 0;
#endif

/*
   Processing section
*/
typedef struct ProcSection {
    char *sectionName;
    Lexeme * (*scanner)();
    void (*init)();
} ProcSection;

Lexeme *ProcDummy();
Lexeme *ProcDefines();
Lexeme *ProcParams();
Lexeme *ProcModifiers();
Lexeme *ProcKeysyms();
Lexeme *ProcButtons();
Lexeme *ProcXlate();
Lexeme *ProcRGBDef();
Lexeme *ProcPixels();
Lexeme *ProcKeyCtrl();

#ifdef XKB
Lexeme *ProcXkbNames();
#endif

void InitDummy();
void InitDefines();
void InitModifiers();
void InitKeysyms();
void InitButtons();
void InitXlate();
void InitRGBDef();
void InitPixels();
void InitKeyCtrl();

#ifdef XKB
void InitXkbNames();
#endif

ProcSection sections[] = {
    {"", ProcDummy, InitDummy},
    {"definitions", ProcDefines, InitDefines},
    {"parameters", ProcParams, InitDummy},
    {"modifiers", ProcModifiers, InitModifiers},
    {"keysyms", ProcKeysyms, InitKeysyms},
    {"buttons", ProcButtons, InitButtons},
    {"translations", ProcXlate, InitXlate},
    {"rgbdefs", ProcRGBDef, InitRGBDef},
    {"pixels", ProcPixels, InitPixels},
    {"keyctrl", ProcKeyCtrl, InitKeyCtrl},
#ifdef XKB
    {"xkb", ProcXkbNames, InitXkbNames},
#endif
    {NULL, NULL, NULL}
};
ProcSection *curSection = sections;    

/*
   Defines
*/
SymtabHead *defines = NULL;

/*
   Parameters
*/
typedef char *xsco_pointer;
typedef struct Parameter {
    char *name;
    enum {ParEnd, ParShort, ParLong, ParString} type;
    xsco_pointer ptr;
} Parameter;

short maxKeysyms = 10;
short mapSize = 256;
short maxButtons = 5;

Parameter params[] = {
    {"ScreenWidth", ParShort, (xsco_pointer)&config.screenWidth},
    {"ScreenHeight", ParShort, (xsco_pointer)&config.screenHeight},
    {"maxKeySyms", ParShort, (xsco_pointer)&maxKeysyms},
    {"mapSize", ParShort, (xsco_pointer)&mapSize},
    {"maxButtons", ParShort, (xsco_pointer)&maxButtons},
    {NULL, ParEnd, NULL}
};
    
/*
   Scancode translation table
*/
int numXlate = 0;
int maxXlate = 0;
ScanTranslation *XlateTable = NULL;

/*
   Color definitions
*/

#define RGB_MASK	0xff00

SymtabHead *RGBTab = NULL;
PixelValue *PixTab = NULL;
int numPixels = 0;
int maxPixels = 0;
/*
   The indices of pixel definition types must match the PV_
   definitions in config.h
*/
char *pixDefType[] = {"temp", "static", "white", "black"};

char *outFile = ".Xsco.cfg";
    
void ProcessFile();    
void WriteConfigFile();

main(ac, av)
int ac;
char *av[];
{
    int argx;
    char *arg;
    
    /*
       Create symbol table for #defines
    */
    defines = InitSymtab(128);
    RGBTab = InitSymtab(128);
       
    /*
       Process options
    */
    for (argx=1; argx < ac && *av[argx] == '-'; argx++) {
	arg = av[argx];
	switch (tolower(arg[1])) {
	    case 'o':
		outFile = av[++argx];
		break;
	}
    }
    
    /*
       Process file names on command line in turn
    */
    for (; argx<ac; argx++) {
	ProcessFile(av[argx]);
    }
    
    /*
       Write configuration file
    */
    WriteConfigFile(outFile);
}

/*
   Get next character from input file.  Called from LexStream.  We
   generate EOF to terminate each line.
*/
int
newChar()
{
    int c;
    
    if (!eof)
	c = getc(curFile);
    else
	c = EOF;
    
    if (c == EOF)
	eof++;
    
    if (c == '\n') {
	lineno++;
	c = EOF;
    }
    
    return c;
}
   
/*
   Print an error message in the form:
   
   filename(lineno) : message
*/
void
ErrorMsg(fmt)
ErrorMessage fmt;
{
    fprintf(stderr, "%s(%d) : ", curFileName, lineno);
    vfprintf(stderr, fmt, (va_list)(&fmt+1));
}
   
/*
   Procedure to process each source file
*/
void
ProcessFile(fileName)
char *fileName;
{
    extern ErrorMessage errFile;
    Lexeme *lex;
    Lexeme *list;
    Lexeme *leftOvers;
    FILE *saveFile;
    char *saveFileName;
    int saveLineno;
    TokenType endType;
    SymtabEntry *sym;
    
    /*
       Push current file
    */
    saveFile = curFile;
    saveFileName = curFileName;
    saveLineno = lineno;
    
    /*
       Open new file
       */
    if ((curFile = fopen(fileName, "r")) == NULL) {
        char libFile[NL_MAXPATHLEN];
        sprintf(libFile, "%s/%s/%s", LIBDIR, "xsconfig", fileName);
        if ((curFile = fopen(libFile, "r")) == NULL) {
            ErrorMsg(errFile, fileName, Strerror(errno));
            exit(1);
        }
    }
    curFileName = fileName;
    lineno = 0;
    
    /*
       Process the file
    */
    while (!eof) {
	
	/*
	   Read a line from the file
	*/

#ifdef XKB
	list = LexStream(" \t\n\r", "=,;:", "\"\"''<>[]{}", newChar);
#else
	list = LexStream(" \t\n\r", "=,;:", "\"\"''<>[]{}()", newChar);
#endif
	
	/*
	   Scan lexemes.  If we hit a semicolon break, fake end of
	   list.  If not in definitions section, try to convert
	   TokWords to numbers or strings.
	*/
	lex = list;
	do {
	    endType = lex->type;
	    switch (endType) {
		
		case TokBreak:
		    if (lex->value.brk == ';')
			lex->type = TokEnd;
		    break;
		    
		case TokWord:
		    if (curSection->scanner != ProcDefines) {
			if (sym = LookupSymbol(defines, lex->value.str)) {
			    free(lex->value.str);
			    if (sym->pval) {
				lex->value.str = StrDup(sym->pval);
				lex->type = TokString;
			    }
			    else {
				lex->value.num = sym->lval;
				lex->type = TokNumber;
			    }
			}
		    }
		    break;
		    
	    }
	    
	} while (lex++->type != TokEnd);
	lex--;				/* backup to TokEnd */
	
	/*
	   Try to do something rational with the token list
	*/
	if (list->type == TokString && list->lead == '[') {
	       
	    /*
	       lookup new processing section
	    */
	    for (curSection=sections; curSection->sectionName; curSection++) {
		if (strcmp(curSection->sectionName, list->value.str) == 0) {
		    break;
		}
	    }
	    if (curSection->sectionName == NULL) {
		extern ErrorMessage errBadSection;
		ErrorMsg(errBadSection, list->value.str);
		exit(1);
	    }
	    else {
		   
		/*
		   Initialize the section
		*/
		(*curSection->init)();
	    }
	}
	else {
	       
	    if (list->type != TokEnd) {
		/*
		   Check for #includes
		*/
		if (list->type == TokWord && 
		    strcmp(list->value.str, "#include") == 0) {
			if (list[1].type == TokString || list[1].type == TokWord) {
			    /*
			       Let's get recursive
			    */
			    leftOvers = &list[2];
			    ProcessFile(list[1].value.str);
			}
			else {
			    extern ErrorMessage errSyntax;
			    ErrorMsg(errSyntax);
			    exit(1);
			}
		}
		else {
		    
		    /*
		       Call scanner to parse the token list
		    */
		    leftOvers = (*curSection->scanner)(list);
		}
		if (leftOvers && leftOvers->type != TokEnd) {
		    extern ErrorMessage errLeftOvers;
		    ErrorMsg(errLeftOvers);
		}
	    }
	}
	
	/*
	   Restore the end token and free the token list
	*/
	lex->type = endType;
	FreeLex(list);
    }
    
    /*
       Back off to previous file
    */
    fclose(curFile);
    curFile = saveFile;
    curFileName = saveFileName;
    lineno = saveLineno;
    eof = 0;
}
   
/*
   Dummy initialization for those sections that need no
   initialization procedure.
*/
void
InitDummy()
{
}

/*
   Dummy processing section.  Treat everything as commentary.
*/
Lexeme *
ProcDummy(list)
Lexeme *list;
{
    return NULL;
}

/*
   Initialize definitions section
*/
void
InitDefines()
{
}

/*
   Process #defines
*/
Lexeme *
ProcDefines(list)
Lexeme *list;
{
    extern ErrorMessage errSyntax;
       
    /*
       Treat any line that does not begin with "#define" as commentary.
    */
    if (list[0].type != TokWord || strcmp(list[0].value.str, "#define") != 0) {
	return NULL;
    }
    
    /*
       Only legal defines are:
       
	  #define symbol
	  #define symbol number
          #define symbol string
       
       We ignore the first form, and there is currently no real use for
       the last form.
    */
    if (list[1].type != TokWord) {
	ErrorMsg(errSyntax);
	return NULL;
    }
    switch (list[2].type) {
	
	case TokNumber:
	    AddSymbol(defines, list[1].value.str, list[2].value.num, NULL);
	    break;
	    
	case TokString:
	    AddSymbol(defines, list[1].value.str, 0L, StrDup(list[2].value.str));
	    break;
	    
	case TokEnd:
	    return NULL;
	    
	default:
	    ErrorMsg(errSyntax);
	    return NULL;
    
    }
       
    /*
       If next token is an open comment, assume no garbage on line
    */
    if (list[3].type == TokWord && strncmp(list[3].value.str, "/*", 2) == 0)
	return NULL;
    else
	return &list[3];
}

/*
   Process parameters section
*/
Lexeme *
ProcParams(list)
Lexeme *list;
{
    extern ErrorMessage errSyntax;   
    extern ErrorMessage errParamName;
    extern ErrorMessage errParamType;
    Parameter *par;
    
    
    /*
       Check syntax.  Must be:
       
         name = value
    */
    if (list[0].type != TokWord ||
	list[1].type != TokBreak || list[1].value.brk != '='||
	list[2].type == TokEnd) {
	    ErrorMsg(errSyntax);
	    return NULL;
    }
    
    /*
       Look up parameter and make assignment
    */
    for (par=params; par->type != ParEnd; par++) {
	if (strcmp(par->name, list[0].value.str) == 0) {
	    switch (par->type) {
		
		case ParShort:
		case ParLong:
		    if (list[2].type == TokNumber) {
			if (par->type == ParShort)
			    *(short *)par->ptr = list[2].value.num;
			else
			    *(long *)par->ptr = list[2].value.num;
		    }
		    else {
			ErrorMsg(errParamType);
		    }
		    break;
		    
		case ParString:
		    if (list[2].type = TokString) {
			*(char **)par->ptr = StrDup(list[2].value.str);
		    }
		    else {
			ErrorMsg(errParamType);
		    }
		    break;
		    
	    }
	    break;
	}
    }
    
    if (par->type == ParEnd) {
	ErrorMsg(errParamName, list[0].value.str);
    }
    
    return &list[3];
}
		    
/*
   Initialize modifier map
*/
void
InitModifiers()
{
    int i;
    
    if (modifierMap != NULL)
	FreeMem(modifierMap);
    
    modifierMap = (unsigned char *)AllocMem(mapSize);
    for (i=0; i<mapSize; i++)
	modifierMap[i] = NoSymbol;
}

/*
   Process modifier map entries
*/
Lexeme *
ProcModifiers(list)
Lexeme *list;
{
    extern ErrorMessage errSyntax;
    extern ErrorMessage errScanCode;
    extern ErrorMessage errModName;
    static struct {
	char * name;
	int value;
    } modNames[] = {
	{"Shift", ShiftMask},
	{"Lock", LockMask},
	{"Control", ControlMask},
	{"Mod1", Mod1Mask},
	{"Mod2", Mod2Mask},
	{"Mod3", Mod3Mask},
	{"Mod4", Mod4Mask},
	{"Mod5", Mod5Mask},
	{NULL, 0}
    };
    int i;
       
    /*
       Check syntax
         number: ModifierName
    */
    if (list[0].type != TokNumber || 
	list[1].type != TokBreak || list[1].value.brk != ':' ||
	list[2].type != TokWord) {
	    ErrorMsg(errSyntax);
	    return NULL;
    }
    
    /*
       Check for valid key number
    */
    if ((unsigned long)list[0].value.num >= 255) {
	ErrorMsg(errScanCode, mapSize);
	return NULL;
    }
    
    /*
       Look up modifier name
    */
    for (i=0; modNames[i].name; i++) {
	if (strcmp(list[2].value.str, modNames[i].name) == 0) {
	    modifierMap[list[0].value.num] = modNames[i].value;
	    return &list[3];
	}
    }
    
    ErrorMsg(errModName);
    return &list[3];
}

/*
   Initialize Keysyms table
*/
void
InitKeysyms()
{
    int entries;
    int i;
    
    if (keysymMap != NULL) 
	FreeMem(keysymMap);
    
    entries = mapSize * maxKeysyms;
    keysymMap = (KeySym *)AllocMem(entries * sizeof(KeySym));
    for (i=0; i<entries; i++)
	keysymMap[i] = NoSymbol;
    
    config.keymapWidth = 0;
    config.minScan = mapSize;
    config.maxScan = 0;
}
   
/*
   Process keysyms
*/
Lexeme *
ProcKeysyms(list)
Lexeme *list;
{
    extern ErrorMessage errSyntax;
    extern ErrorMessage errScanCode;
    extern ErrorMessage errNumKeysyms;
    extern ErrorMessage errKeysymName;
    KeySym *row;
    int cnt;
    unsigned int scan;
    Lexeme *lex;
    
    /*
       Check syntax:
         number : ...
    */
    if (list[0].type != TokNumber ||
	list[1].type != TokBreak || list[1].value.brk != ':') {
	    ErrorMsg(errSyntax);
	    return NULL;
    }
    
    /*
       Calculate row pointer
    */
    scan = list[0].value.num;
    if (scan >= mapSize) {
	ErrorMsg(errScanCode, mapSize);
	return NULL;
    }
    row = keysymMap + (maxKeysyms * scan);
    
    /*
       Set keysyms
    */
    for (lex=&list[2], cnt=0; lex->type == TokNumber && cnt < maxKeysyms;
	lex++, cnt++) {
	    row[cnt] = lex->value.num;
    }
    if (lex->type == TokNumber) {
	ErrorMsg(errNumKeysyms, maxKeysyms);
	lex = NULL;
    }
    if (lex->type == TokWord) {
	ErrorMsg(errKeysymName, lex->value.str);
	lex = NULL;
    }
    
    /*
       Check boundaries
    */
    if (scan < config.minScan) config.minScan = scan;
    if (scan > config.maxScan) config.maxScan = scan;
    if (cnt > config.keymapWidth) config.keymapWidth = cnt;
    
    return lex;
}
	
/*
   Initialize buttons
*/
void
InitButtons()
{
    int i;
    
    if (buttonMap != NULL) 
	FreeMem(buttonMap);
    buttonMap = (unsigned char *)AllocMem(maxButtons+1);
    for (i=0; i<=maxButtons; i++)
	buttonMap[i] = i;
}

/*
   Process button map
*/
Lexeme *
ProcButtons(list)
Lexeme *list;
{
    extern ErrorMessage errMaxButtons;
    Lexeme *lex;
    int cnt;
    
    /*
       Just process all the TokNumbers until we hit the end or
       maxButtons. 
    */
    for (lex=list, cnt=1; lex->type == TokNumber && cnt <= maxButtons;
	lex++, cnt++) {
	    buttonMap[cnt] = lex->value.num;
    }
    if (lex->type == TokNumber) {
	ErrorMsg(errMaxButtons, maxButtons);
	lex = NULL;
    }
    
    config.buttonSize = cnt;
    return lex;
}

/*
   Initialize scancode translation table
*/
void
InitXlate()
{
}

/*
   Process scancode translations
*/
Lexeme *
ProcXlate(list)
Lexeme *list;
{
    extern ErrorMessage errSyntax;
    ScanTranslation *xlate;   
    
    /*
       Check syntax
          oldscancode=newscancode
    */
    if (list[0].type != TokNumber ||
        list[1].type != TokBreak || list[1].value.brk != '=' ||
	list[2].type != TokNumber) {
	    ErrorMsg(errSyntax);
	    return NULL;
    }
    
    /*
       Add the translation entry
    */
    if (numXlate >= maxXlate) {
	maxXlate += 20;
	XlateTable = (ScanTranslation *)ReallocMem(XlateTable, maxXlate*sizeof(ScanTranslation));
    }
    xlate = &XlateTable[numXlate++];
    xlate->oldScan = list[0].value.num;
    xlate->newScan = list[2].value.num;
    
    return &list[3];
}

/*
   InitRGBDef - initialize rgb definitions section
*/
void
InitRGBDef()
{
}	

/*
   ProcRGBDef - process and RGB definition record
*/
Lexeme *
ProcRGBDef(list)
Lexeme *list;
{
    extern ErrorMessage errSyntax;
    extern ErrorMessage errRGBValue;
    extern ErrorMessage errRGBName;
    char colorname[256];   
    Lexeme *lex;
    long rgb;
    int i;
    
    /* 
       Comments in rgb.txt start with #.
    */
    if (list[0].type == TokWord && list[0].value.str[0] == '#')
	return NULL;
    
    /*
       Pick up red, green and blue values and pack them into a long
    */
    for (i=0; i<3; i++) {
	if (list[i].type != TokNumber) {
	    ErrorMsg(errSyntax);
	    return NULL;
	}
	if (list[i].value.num & 0xffffff00) {
	    ErrorMsg(errRGBValue);
	    return NULL;
	}
    }
    rgb = (list[0].value.num << 16) | (list[1].value.num << 8) | 
						    list[2].value.num;
    
    colorname[0] = 0;
    for (lex = &list[3]; lex->type == TokWord; lex++) {
	strcat(colorname, lex->value.str);
    }
    if (colorname[0] == 0) {
	ErrorMsg(errRGBName);
	return NULL;
    }
    strlwr(colorname);
	
    AddSymbol(RGBTab, colorname, rgb, NULL);
    
    return lex;
}

/*
   Initialize pixel definitions
*/
void
InitPixels()
{
    if (maxPixels == 0) {
	maxPixels += 16;
	PixTab = (PixelValue *)AllocMem(maxPixels * sizeof(PixelValue));
    }
}

/*
   AddPixel - add a pixel definition
*/
void
AddPixel(pix)
PixelValue *pix;
{
    if (numPixels >= maxPixels) {
	maxPixels += 16;
	PixTab = (PixelValue *)ReallocMem(PixTab, sizeof(PixelValue) * maxPixels);
    }
    PixTab[numPixels++] = *pix;
}

/*
   Process pixel definitions
   
   Syntax:
       pixelNumber: redval greenval blueval [temp|static|white|black]
       pixelNumber: "color name" [temp|static|white|black]
*/
Lexeme *
ProcPixels(list)
Lexeme *list;
{
    extern ErrorMessage errSyntax;
    extern ErrorMessage errColorName;
    Lexeme *lex;
    PixelValue pixTemp;
    char *sp;
    char *dp;
    char **cpp;
    char colorname[256];
    SymtabEntry *sym;
    int i;
    
    /*
       Check for "pixelNumber:"
    */
    if (list[0].type != TokNumber ||
	list[1].type != TokBreak || list[1].value.brk != ':') {
	    ErrorMsg(errSyntax);
	    return NULL;
    }
    pixTemp.pixel = list[0].value.num;
    
    /*
       Look for color name or RGB values
    */
    lex = &list[2];
    if (lex[0].type == TokString) {
	for (sp=lex[0].value.str, dp=colorname; *sp; sp++) {
	    if (*sp != ' ' && *sp != '\t')
		*dp++ = *sp;
	}
	*dp = 0;
	strlwr(colorname);
	if ((sym = LookupSymbol(RGBTab, colorname)) == NULL) {
	    ErrorMsg(errColorName, lex[0].value.str);
	    return NULL;
	}
	pixTemp.red = (sym->lval >> 8) & RGB_MASK;
	pixTemp.green = sym->lval & RGB_MASK;
	pixTemp.blue = (sym->lval << 8) & RGB_MASK;
	lex++;
    }
    else if (lex[0].type == TokNumber && lex[1].type == TokNumber &&
	     lex[2].type == TokNumber) {
	pixTemp.red = (lex[0].value.num << 8) & RGB_MASK;
	pixTemp.green = (lex[1].value.num << 8) & RGB_MASK;
	pixTemp.blue = (lex[2].value.num << 8) & RGB_MASK;
	lex += 3;
    }
    else {
	ErrorMsg(errSyntax);
	return NULL;
    }
    
    /*
       Look for definition type:
       
         temp - initialize to this color, but leave unallocated
         static - initialize and allocate to server
         white - this is the static cell for WhitePixel
         black - this is the static cell for BlackPixel
    */
    pixTemp.type = PV_StaticPixel;
    if (lex[0].type == TokWord) {
	strlwr(lex[0].value.str);
	for (cpp=pixDefType; *cpp; cpp++) {
	    if (strcmp(*cpp, lex[0].value.str) == 0) {
		pixTemp.type = cpp - pixDefType;
		break;
	    }
	}
	if (*cpp == NULL) {
	    ErrorMsg(errSyntax);
	    return NULL;
	}
	lex++;
    }
    
    /*
       Add the pixel definition
    */
    AddPixel(&pixTemp);
    
    return lex;
    
}

/*
   Initialize Keyboard Control record
*/
void
InitKeyCtrl()
{
    if (keyCtrlRec == NULL)
	keyCtrlRec = (unsigned char *)AllocMem(KEY_CTRL_SIZE);
    
    memset(keyCtrlRec, 0, KEY_CTRL_SIZE);
    config.keyctrlSize = KEY_CTRL_SIZE;
}

/*
   Process keyboard control
*/
Lexeme *
ProcKeyCtrl(list)
Lexeme *list;
{
    extern ErrorMessage errSyntax;
    extern ErrorMessage errBadKeycode;
    extern ErrorMessage errBadKeyCtrl;
    unsigned int index;
    int i;
    static struct {
	char *name;
	short flag;
    } controlTable[] = {
	{"CapsLock", KF_CAPS_LIGHT},
	{"NumLock", KF_NUM_LIGHT},
	{"ScrollLock", KF_SCROLL_LIGHT},
	{"PseudoLock", KF_TOGGLE_KEY},
	{"XIgnore", KF_IGNORE_KEY},
	{"BiosIgnore", KF_SKIP_BIOS},
	{NULL, 0}
    };
	
    /*
       Make sure line begins with "number:"
    */
    if (list[0].type != TokNumber ||
	list[1].type != TokBreak || list[1].value.brk != ':') {
	    ErrorMsg(errSyntax);
	    return NULL;
    }
    
    /*
       Calculate index into keyCtrlRec.
    */
    index = list[0].value.num;
    switch (index & 0xff00) {
	case 0:
	    break;
	    
	case 0xe000:
	    index = (index & 0xff) + KF_E0_OFFSET;
	    break;
	    
	case 0xe100:
	    index = (index & 0xff) + KF_E1_OFFSET;
	    break;
		
	default:
	    ErrorMsg(errBadKeycode);
	    return NULL;
    }
    list += 2;
    
    /*
       Process key control flags
    */
    while (list[0].type == TokWord) {
	for (i=0; controlTable[i].name; i++) {
	    if (strcmp(controlTable[i].name, list[0].value.str) == 0)
		break;
	}
	if (controlTable[i].name == NULL) {
	    ErrorMsg(errBadKeyCtrl, list[0].value.str);
	    return NULL;
	}
	keyCtrlRec[index] |= controlTable[i].flag;
	list++;
    }
    
    return list;
}


#ifdef XKB
		    
/*
   Initialize xkb component names.
*/
void
InitXkbNames()
{
    XkbComponentNamesRec xkbNamesHdr;
    
    if (xkbNames != NULL)
	FreeMem(xkbNames);

    xkbNamesSize = sizeof(XkbComponentNamesRec);
    xkbNames = (char *)AllocMem(xkbNamesSize);
    memset(xkbNames, 0, xkbNamesSize);
}

/*
   Process xkb component names entries.
*/
Lexeme *
ProcXkbNames(list)
Lexeme *list;
{
    extern ErrorMessage errSyntax;
    char *compName, *compValue;
    int compLen;
    XkbComponentNamesPtr pXkbHeader;

    if (xkbNames == 0)
        InitXkbNames();

    pXkbHeader = (XkbComponentNamesPtr)xkbNames;
       
    /*
       Check syntax
         number: ModifierName
    */
    if (list[0].type != TokWord || 
	list[1].type != TokBreak || list[1].value.brk != '=' ||
	list[2].type != TokWord) {
	    ErrorMsg(errSyntax);
	    return NULL;
    }
    else
    {
        compName = list[0].value.str;
        compValue = list[2].value.str;
        compLen = strlen(compValue);
        if (compLen <= 0)
            return NULL;
    }
    
    if (!strcmp(compName, "keymap")) {
        pXkbHeader->keymap = (char *)xkbNamesSize;
    }
    else
    if (!strcmp(compName, "keycodes")) {
        pXkbHeader->keycodes = (char *)xkbNamesSize;
    }
    else
    if (!strcmp(compName, "types")) {
        pXkbHeader->types = (char *)xkbNamesSize;
    }
    else
    if (!strcmp(compName, "compat")) {
        pXkbHeader->compat = (char *)xkbNamesSize;
    }
    else
    if (!strcmp(compName, "symbols")) {
        pXkbHeader->symbols = (char *)xkbNamesSize;
    }
    else
    if (!strcmp(compName, "geometry")) {
        pXkbHeader->geometry = (char *)xkbNamesSize;
    }
    else
    {
        return NULL;
    }

    xkbNames = (char *)ReallocMem(xkbNames, xkbNamesSize + compLen + 1);
    memset(xkbNames + xkbNamesSize, 0, compLen + 1);
    pXkbHeader = (XkbComponentNamesPtr)xkbNames;
    strncpy(xkbNames+xkbNamesSize, compValue, compLen);
    xkbNamesSize += (compLen + 1);
    
    return &list[3];
}

#endif /* XKB */


/*
   Write out the configuration file
*/
void
WriteConfigFile(filename)
char *filename;
{
    extern ErrorMessage errFile;
    FILE *configFile;
    int i;
    KeySym *row;
    int scanSize;
    int scanCodes;
    int modmapSize;
    char czero = '\0';
    
    /*
       "row" is the pointer to the keysym row of minScan.
    */
    config.scanBias = 0;
    row = keysymMap + (config.minScan * maxKeysyms);
    modmapSize = config.maxScan + 1;
    scanCodes = config.maxScan + 1 - config.minScan;
    scanSize = config.keymapWidth * sizeof(KeySym);
    
    /*
       Fill in ConfigRec
    */
    config.headerSize = sizeof(config);
    config.buttonOffset = config.headerSize;
    /* config.buttonSize set in ProcButtons */
    config.modmapOffset = config.buttonOffset + config.buttonSize;
    config.modmapSize = config.maxScan + 1;
    config.keymapOffset = config.modmapOffset + config.modmapSize;
    config.keymapSize = scanSize * scanCodes;
    config.translateOffset = config.keymapOffset + config.keymapSize;
    config.translateSize = numXlate * sizeof(ScanTranslation);
    config.pixelOffset = config.translateOffset + config.translateSize;
    config.pixelSize = sizeof(PixelValue) * numPixels;
    config.keyctrlOffset = config.pixelOffset + config.pixelSize;
    /* config.keyctrlSize is set in InitKeyCtrl */

#ifdef XKB
    config.xkbNamesOffset = config.keyctrlOffset + config.keyctrlSize;
    config.xkbNamesSize = xkbNamesSize;
#endif
    
    
    /*
       Open the config file
    */
    if ((configFile = fopen(filename, "wb")) == NULL) {
	ErrorMsg(errFile, filename, Strerror(errno));
	return;
    }
    
    /*
       Some of this is easy
    */
    fwrite(&config, sizeof(config), 1, configFile);
    fwrite(buttonMap, config.buttonSize, 1, configFile);
    
    /*
       We need to write config.scanBias bytes of zero in front of the
       modifier map.
    */
    for (i=0; i<config.scanBias; i++) {
	fwrite(&czero, 1, 1, configFile);
    }
    fwrite(modifierMap, modmapSize, 1, configFile);
    
    /*
       Write out the first config.keymapWidth entries in each row of
       the keysym map.
    */
    for (i=config.minScan; i<=config.maxScan; i++) {
	fwrite(row, sizeof(KeySym), config.keymapWidth, configFile);
	row += maxKeysyms;
    }
    
    /*
       Write out scancode translation table
    */
    if (config.translateSize) {
	fwrite(XlateTable, config.translateSize, 1, configFile);
    }
    
    /*
       Write pixel definitions
    */
    if (config.pixelSize) {
	fwrite(PixTab, config.pixelSize, 1, configFile);
    }
    
    /*
       Write key control record
    */
    if (config.keyctrlSize) {
	fwrite(keyCtrlRec, config.keyctrlSize, 1, configFile);
    }
    
#ifdef XKB
    /*
       Write xkb component names info
    */
    if (config.xkbNamesSize) {
	fwrite(xkbNames, config.xkbNamesSize, 1, configFile);
    }
#endif

   /*
       That's all folks.
    */
    fclose(configFile);
}

/*
	No strlwr on SlimeIX
*/
void
strlwr(str)
char *str;
{
	for(; *str; str++) *str = tolower(*str);
}

/*
	No Strerror on SlimeIX
*/
char *
Strerror(code)
{
	extern char *sys_errlist[];
	extern int sys_nerr;

	if (code < sys_nerr)
		return sys_errlist[code];
	else
		return("Unknown Error");
}

