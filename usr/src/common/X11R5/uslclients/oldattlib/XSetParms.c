#ifndef	NOIDENT
#ident	"@(#)oldattlib:XSetParms.c	1.1"
#endif
/*
 XSetParms.c (C source file)
	Acc: 575323971 Fri Mar 25 15:12:51 1988
	Mod: 574629704 Thu Mar 17 14:21:44 1988
	Sta: 574629704 Thu Mar 17 14:21:44 1988
	Owner: 2011
	Group: 1985
	Permissions: 664
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/* XSetParms  ---   X-Windows Parameter Initialization Routine */
/*                                                             */
/* Description: This routine parses the argv arrary passed and */
/*              initializes variables 'assigned' to the valid  */
/*              switches.  Before parsing the command line the */
/*              users .Xdefaults entries are consulted (via    */
/*              XGetDefault) to see if any predefined default  */
/*              settings exist.  If they do they are applied   */
/*              before the command line switches are handled.  */
/*              This routine also opens the display and handles*/
/*              colormap lookup for the client.                */

/* Author: Richard J. Smolucha                                */
/* Creation Date: July 23, 1987                               */
/* Last Modification: December 7, 1987 (for Version 11)       */
/*      Modification: August 20, 1987 (documentation)         */

#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include "XSetParms.h"

Display * XOpenDisplay();
typedef void (*PFV) ();

double atof();
int atoi();
long atol();

char * XGetDefault();

void XExtractDisplay();
char * ExtractDisplayName();
void XGetDefaults();
int  ParseArgs();
int  XSetupColor();
int  XSetupFonts();
int  SetupFiles();
#define PGMNAME argv[0]
/*
int XSetParms                (argc,argv,switches,Syntax,dpy,Geometry,DispName)
int SetParms                 (argc,argv,switches,Syntax)
void XExtractDisplay         (&argc,argv,DispName,dpy);
void XGetDefaults            (argc,argv,switches,dpy);
int ParseArgs                (argc,argv,switches,(PFV)Syntax);
int XSetupColor              (argc,argv,switches,(PFV)Syntax,dpy);
int XSetupFonts              (argc,argv,switches,(PFV)Syntax,dpy);
int  SetupFiles              (argc,argv,switches,(PFV)Syntax);
*/

int SetParms(argc,argv,switches,Syntax)
int    argc;
char * argv[];
struct XSwitches switches[];
PFV    Syntax;
{
int retval = 0;

retval += ParseArgs               (argc,argv,switches,(PFV)Syntax);
retval += SetupFiles              (argc,argv,switches,Syntax);

return retval;

} /* end of SetParms */
int XSetParms(argc,argv,switches,Syntax,dpy,Geometry,DispName)

int    argc;
char * argv[];
struct XSwitches switches[];
PFV    Syntax;
Display ** dpy;
char ** Geometry;
char ** DispName;
{
int retval = 0;

XExtractDisplay         (&argc,argv,DispName,dpy);
XGetDefaults            (argc,argv,switches,*dpy);

retval += ParseArgs                (argc,argv,switches,(PFV)Syntax);

retval += XSetupColor              (argc,argv,switches,(PFV)Syntax,*dpy);
retval += XSetupFonts              (argc,argv,switches,(PFV)Syntax,*dpy);
retval +=  SetupFiles              (argc,argv,switches,(PFV)Syntax);

return retval;

} /* end of XSetParms */
/* XGetDefaults ---  get user defaults using XGetDefault       */
/*                                                             */
/* Description: This routine calls XGetDefault using the       */
/*              default name stored in the switches structure  */
/*              for each switch in the structure and assigns   */
/*              the appropriate value to the appropriate       */
/*              variable if a default is returned.             */

void XGetDefaults(argc,argv,switches,dpy)

Display * dpy;
int    argc;
char * argv[];
struct XSwitches switches[];
{
register j = 0;
char * DefaultPtr;

   for (j = 0; switches[j].swtype; j++)
      {
      if ((DefaultPtr = XGetDefault(dpy, PGMNAME,switches[j].defname)) != NULL)
	 HandleOption(DefaultPtr,
	    switches[j].swtype, switches[j].variable, &switches[j].pgmdefault);
      } /* end of for */

} /* end of XGetDefaults */
/* ParseArgs  ---   'parses' command-line switches             */
/*                                                             */
/* Description: This routine examines the command-line argu-   */
/*              ments and scans the switches structure to      */
/*              find the type and variable addresses assoc-    */
/*              iated with each switch.  It then assigns the   */
/*              appropriate value to the appropriate variable. */
/*              The user supplied Syntax routine is called if: */
/*                                                             */
/*              1. The switch is not found.                    */
/*              2. The field is of type COLOR, FONT, or STR    */
/*                 and no string follows (i.e., the switch     */
/*                 is the last argument) or                    */
/*                 The field is of type NUMB and the switch    */
/*                 is the last argument.                       */

int ParseArgs(argc,argv,switches,Syntax)

int    argc;
char * argv[];
struct XSwitches switches[];
PFV    Syntax;
{
register i = 0;
register j = 0;
int found = 0;
int retval = 0;

for (i = 1; i < argc; i++)
   {
   for (j = 0, found = 0; switches[j].swtype && !found; j++)
      {
      if (found = !strncmp(argv[i],switches[j].sw,strlen(argv[i])) &&
			   ((int)strlen(argv[i]) >= switches[j].len))
         switch (switches[j].swtype)
         {
         case FLIP:  
         case CLEAR:
         case SET: 
	             HandleOption("on", switches[j].swtype, 
			switches[j].variable, &switches[j].pgmdefault);
		     break;
	 default:    if (++i >= argc)
			{
			retval++;
                        (*Syntax)("Switch %s requires an argument!",
			   PGMNAME, 2, 0, 1, -1, argv[i], NULL);
			}
                     else
	                HandleOption(argv[i], switches[j].swtype, 
			   switches[j].variable, &switches[j].pgmdefault);
		     break;
         }
      }

   if (!found) 
      {
      retval++;
      (*Syntax)("Unknown switch %s ignored!", PGMNAME, 1,0, 1,0, argv[i],NULL);
      }
   } 

return retval;

} /* end of ParseArgs */
void  XExtractDisplay(argc, argv, DispName, dpy)

int  *  argc;
char *  argv[];
char ** DispName;
Display ** dpy;
{
char * DefaultPtr;

if ((DefaultPtr = ExtractDisplayName(argc,argv)) != NULL)
    *DispName = DefaultPtr;

if (dpy)
   {
   Display * d;
   if ((d = XOpenDisplay(*DispName)) == NULL)
      {
      fprintf (stderr,"%s: Can't open display [%s] [%s].",
   	           PGMNAME,XDisplayName(*DispName),*DispName ? *DispName : "default");
      exit(1);
      }
   else
      *dpy = d;
   }

} /* end of XExtractDisplay */
int XSetupFonts(argc,argv,switches,Syntax,dpy)

int argc;
char * argv[];
struct XSwitches switches[];
PFV    Syntax;
Display * dpy;

{
register i = 0;
int retval = 0;

for (i=0; switches[i].swtype; i++)
   { 
   if (switches[i].swtype == FONT)
      {
      XFontStruct * xfs;
      if ( (xfs = XLoadQueryFont(dpy, switches[i].pgmdefault)) != NULL)
	  *(XFontStruct **)switches[i].variable = xfs;
      else
	 {
	 retval++;
	 switches[i].errorvalue = 5;
         (*Syntax)("Can't query font %s!",
	    PGMNAME, 5, 0, 0, 1, switches[i].pgmdefault, NULL);
	 }
      }
   }

return retval;

} /* end of XSetupFonts */
int SetupFiles(argc,argv,switches,Syntax)

int argc;
char * argv[];
struct XSwitches switches[];
PFV    Syntax;

{
register i = 0;
int retval = 0;

for (i=0; switches[i].swtype; i++)
   { 
   if (switches[i].swtype == INPUT || switches[i].swtype == OUTPUT)
      {
      FILE ** p = (FILE **)  switches[i].variable;
      char * mode = switches[i].swtype == INPUT ? "r" : "w";
      char * modelong = switches[i].swtype == INPUT ? "reading" : "writing";

      if (!(*p = fopen(switches[i].pgmdefault,mode)))
	 {
	 retval++;
	 switches[i].errorvalue = 3;
         (*Syntax)("Can't open file %s for %s!",
	    PGMNAME, 3, 0, 0, 1, switches[i].pgmdefault, modelong, NULL);
	 }
      }
   }

return retval;

} /* end of SetupFiles */
int XSetupColor(argc,argv,switches,Syntax,dpy)

int argc;
char * argv[];
struct XSwitches switches[];
PFV    Syntax;
Display * dpy;

/* set up colors (if we have them) otherwise assume B & W */
{
int retval = 0;

if (DisplayCells(dpy, DefaultScreen(dpy)) > 2)
   {
   register i = 0;
   XColor color;
   Colormap colormap = DefaultColormap(dpy,DefaultScreen(dpy));
   for (i=0; switches[i].swtype; i++)
      { 
      if (switches[i].swtype == COLOR)
	 {
         if ( XParseColor(dpy, colormap, switches[i].pgmdefault, &color))
             {
             XAllocColor( dpy, colormap, &color);
	     *(unsigned long *)switches[i].variable = color.pixel;
             }
         else
	    {
	    retval++;
	    switches[i].errorvalue = 4;
            (*Syntax)("Can't allocate color %s!",
	       PGMNAME,4,0,0,0,switches[i].pgmdefault,NULL);
	    }
	 }
      }
   }

return retval;

} /* end of XSetupColor */
FoundInSetOf(set,value)
char * set[];
char * value;
{
register i;

for (i = 0; set[i] != NULL; i++)
   if (!strcmp(set[i],value)) return 1;

return 0;

} /* end of FoundInSetOf */
HandleOption(value, type, variable, pgmdefault)
char * value;
int type;
char ** variable;
char ** pgmdefault;
{
static char * Falsehoods[] = {"no","NO","No","false","FALSE","False","0","off","OFF","Off",NULL};
static char * Truisms[] = {"yes","YES","Yes","true","TRUE","True","1","on","ON","On",NULL};

*pgmdefault = value;
   switch (type)
      {
      case FLIP: *(int *)variable = (int) !*(int *)variable; break;
      case CLEAR: if (FoundInSetOf(Falsehoods,value))
                     *(int *)variable = TRUE;
                  else
                     if (FoundInSetOf(Truisms,value))
                        *(int *)variable = FALSE;
                  break;
      case SET:   if (FoundInSetOf(Falsehoods,value))
                     *(int *)variable = FALSE;
                  else
                     if (FoundInSetOf(Truisms,value))
                        *(int *)variable = TRUE;
                  break;
      case STRING:           *variable = value;
      case FONT:
      case INPUT:
      case OUTPUT:
      case COLOR:             break;
      case FLOAT:   *(float *)variable = (float) atof(value); break;
      case DOUBLE: *(double *)variable = atof(value); break;
      case INT:       *(int *)variable = atoi(value); break;
      case NUMBER: 
      case LONG:     *(long *)variable = atol(value); break;
      case USER:  break;
      default: break;
   } 

} /* end of HandleOption */
/* XSyntax    ---   X-Windows Syntax routine                  */
/*                                                            */
/* Description: This routine reports errors in usage          */
/*              and describes the usage syntax using          */
/*              an extern char pointer array named            */
/*              Syntax.  This array is generated by the       */
/*              xsp utility.                                  */

/* Author: Richard J. Smolucha                                */
/* Creation Date: July 23, 1987                               */
/* Last Modification: August 20, 1987 (documentation)         */

#include <stdio.h>

extern char *Syntax[];

void XSyntax(format,pgmname,ErrorNumber,ErrorDetail,use,fatal,x1,x2,x3,x4,x5)

char * format;
char * pgmname;
int ErrorNumber;
int ErrorDetail;
int use;
int fatal;
char * x1;
char * x2;
char * x3,x4,x5;

{
/*
          ERROR 1.0 Unknown Switch
          ERROR 2.0 Switch requires an argument
          ERROR 3.0 Can't open file
          ERROR 4.0 Can't allocate color
*/
register i = 0;

if (ErrorNumber >= 0)
   {
   fprintf (stderr,"%s: error %d.%d - ", pgmname, ErrorNumber, ErrorDetail);
   fprintf (stderr,format,x1,x2,x3,x4,x5);
   fprintf (stderr,"\n");
   }

if (use)
   {
   fprintf (stderr,"\nusage: %s\n",pgmname);

   for (i = 0; Syntax[i]; i++)
      fprintf(stderr,"\t%s\n",Syntax[i]);
   }

if (fatal >= 0)
   exit(fatal);

} /* end of XSyntax */

char * ExtractDisplayName (argc, argv)
int *		argc;
char **		argv;
{
	int	n = *argc;
	int	i;
	char *	p;
	char *	display = (char *) 0;

	for (i = 1; i < n; i++)
	{
		if (!strcmp(argv[i], "-display") && i < n)
		{
			display = argv[++i];
			*argc -= 1;
			break;
		}
	}
	for (++i; i < n; ++i)
		argv[i-1] = argv[i];
	return display;
}
