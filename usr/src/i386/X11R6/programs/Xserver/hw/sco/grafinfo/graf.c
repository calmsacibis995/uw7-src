/*
 *      @(#)graf.c	11.5	11/25/97	15:37:48
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Fri Feb 01 19:52:32 PST 1991	pavelr@sco.com
 *	-  Created File
 *	S001	Fri Feb 01 23:11:07 PST 1991	buckm@sco.com
 *	- Refrain from strlwr()'ing class and mode in the grafData
 *	structures; use strcasecmp() to match with them.
 *	S002	Thu Feb 14 15:37:00 PST 1991	pavelr@sco.com
 *	- added XSERVER conditional for some things
 *	S003	Thu Feb 14 15:37:00 PST 1991	pavelr@sco.com
 *	- strip out error routines and put in graferror settings
 *	S004	Sun Feb 17 22:34:26 PST 1991	buckm@sco.com
 *	- when XSERVER map printf to ErrorF; don't use intrinsic i/o.
 *	S005	Thu Apr 25 19:00:00 PST 1991	pavelr@sco.com
 *	- reworked some things to be cleaner, many changes
 *	S006	Mon Aug 26 15:56:17 PDT 1991	pavelr@sco.com
 *	- rewrote most of file for new grafinfo API
 *	S007	Tue Sep 10 1991	pavelr@sco.com
 *	- assorted clean ups from design review
 *	S008	Tue Sep 10 1991	pavelr@sco.com
 *	- added argument passing
 *	S009	Mon Sep 16 1991			pavelr@sco.com
 *	- grafGetFunction added
 *	S010	Mon Sep 30 22:55:11 PDT 1991	mikep@sco.com
 *	- Change strdup() to Xstrdup()
 *	S011	Wed Mar 25 1992			pavelr@sco.com
 *      - added memory management code
 *	S012	Tue Jul 28 07:32:08 PDT 1992	buckm@sco.com
 *	- Fix OP_WAIT.
 *	S013	Thu Sep 03 16:28:59 PDT 1992	hiramc@sco.COM
 *	- declare Xstrdup correctly to avoid compiler warnings
 *	S014	Tue Sep 08 02:43:49 PDT 1992	buckm@sco.com
 *	- Add OP_INT10 support.
 *	- Clean up XSERVER differences.
 *	- Call grafMemManage() at the end of grafParseFile().
 *	- Add grafGetMemInfo() function.
 *	- Memory and ports are now in linked lists,
 *	  and there is a new port structure.
 *	- Memory now has a name. 
 *	- Add AddMem(), AddPort(), and AddPortGroup() functions.
 *	- The S011 read and write mem ops, as well as the new
 *	  int10 op, need access to the grafData structure.
 *	  But, the grafData pointer is not passed into grafRunCode().
 *	  Since grafRunCode() is a linkkit routine with a published
 *	  interface, we can't add a new arg to it.  So, the guts of
 *	  the code interpreter will be in the new grafRunFunction()
 *	  which _will_ have the grafData pointer as an arg.  grafExec()
 *	  will call this new routine instead.  grafRunCode() will now
 *	  call grafRunFunction() passing a NULL grafData pointer.
 *	  The int10 and memory ops will check the pointer before use
 *	  and issue a warning message if NULL.
 *	- Change indentation in grafRunFunction() to make it more readable.
 *	- Muck with grafRunFunction() to make it more efficient.
 *	S015	Tue Oct 27 08:36:50 PST 1992	buckm@sco.com
 *	- Grab V86OPTS before calling VBiosInit().
 *	S016	Mon Nov 02 11:58:48 PST 1992	buckm@sco.com
 *	- Adjust int10 stuff to interface with a vbios daemon.
 *	- V86OPTS becomes VBIOSOPTS and is now an arg to vbInit().
 *	- Change all printf's to Eprint's.
 *	S017	Thu Dec 17 08:20:23 PST 1992	hiramc@sco.COM
 *	- Don't declare inline functions when compiling inline functions
 *	S018	Sun Mar 28 00:58:52 PST 1993	buckm@sco.com
 *	- Add support for callrom op.
 *	- Grab VBMEMFILE to pass to VBiosInit() for debug.
 *	- Change the EFF port group to map 2E8:8 and extensions.
 *	S019	Tue Apr 13 15:02:06 PDT 1993	mikep@sco.com
 *	- Make strings a little bigger to allow for long lists of mode 
 *	names in the grafdev file for supporting up to 32 heads.
 *	S020    Wdn Jun 23 1993 edb@sco.com
 *	- Changes to execute compiled grafinfo files
 *	S021    Wdn Jul 21 1993 edb@sco.com
 *	- Add grafInw()
 *	S022	Wed Oct 19 1994 rogerv@sco.com
 *	- Open /dev/console before doing MAP_CLASS.  If that fails, try
 *	stdout.
 *	S023	Fri Oct 21 1994 rogerv@sco.com
 *	- Remove S022, make the change in /etc/clean_screen instead.  Oops.
 *      S024    Sat Jun 29 00:17:02 GMT 1996 kylec@sco.com
 *      - Use GRAFDIR for location of grafinfo directory.
 *	S025	Mon Nov 11 15:51:06 PST 1996	-	hiramc@sco.COM
 *	- add the call to vbIOEnable in grafVBInit to limit access to
 *	- specified I/O ports
 *      S026    Mon Dec 30 16:45:16 PST 1996 	kylec@sco.com
 *      - use v86 mode support in kernel to execute bios.  If
 *        support is not available then default to interpreter.
 *	S027	Tue Apr 15 10:05:04 PDT 1997	hiramc@sco.COM
 *	- add the CON_MEM_MAPPED calls
 *	S028	Tue Oct 28 14:14:59 PST 1997	hiramc@sco.COM
 *	- moving all mmap and CON_MEM_MAPPED ioctl to MemConMap.c
 *	- and the function memconmap
 *	S029	Tue Nov 11 17:13:53 PST 1997	hiramc@sco.COM
 *	- do not use v86.h in Gemini
 */

/*
	graf.c - some interfaces to the grafData structure
 */

#if defined(usl)
#include <sys/types.h>
#include <sys/unistd.h>		/*	S027	*/
#include <sys/kd.h>
#include <sys/ws/ws.h>		/*	S027	*/
#include	<sys/mman.h>
#else
#include <sys/console.h>  /* S011 */
#endif

/* #include <sys/machdep.h> */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>                            /* S020 */

#include "os.h"
#include "grafinfo.h"
#include "y.tab.h"
#include "dyddx.h"                            /* S020 */
#include "commonDDX.h"                            /* S020 */

#ifdef V86BIOS
#include "v86opts.h"
#if ! defined(usl)				/*	S0xx	*/
#include "v86.h"
#endif 						/*	S0xx	*/
#endif

/* Do some things differently if we are in the server		   S014 */
#ifdef XSERVER
#define	Eprint		ErrorF					/* S016 */
#define malloc(x)	Xalloc(x)
#define free(x)		Xfree(x)
#define strdup(x)	Xstrdup(x)
extern char * Xstrdup( char * );	/*	in server/os/utils.c S013 */
#endif /* XSERVER */

/* default grafinfo files */
#ifndef GRAFDIR
#define GRAFDIR "/usr/lib/grafinfo"
#endif

char *grafdir = GRAFDIR;        /* S024 */
char *grafdev = "grafdev";
char *grafdef = "grafinfo.def";

static struct dynldinfo  grafLdInfo;     /* we better put this into grafData */

#ifndef USE_INLINE_CODE					/*	S017	*/
/* these are used for setting and reading IO registers */
int inp(int port);
void outp(int port, int value);
void outpw(int port, int value);
#endif							/*	S017	*/

#define STRLEN 256						/* S019 */
#define LSTRLEN STRLEN * 8					/* S019 */

/* from the lexical analyzer */
extern FILE	*yyin;

/* for the lexical analyzer */
codeType *grafCode = NULL;

/* the error value */
int graferrno = GEOK;

/* the currently used grafData */
static grafData *cur_g;                         /* S020 */

/* for the yacc parser to set values */
grafData *parsit;

/* die if malloc fails */
void *GrafMalloc (s)
    size_t s;
{
    void *d;
    if (!(d = (void *) malloc (s))) {
        graferrno = GEALLOC;    /* S005 */
        return ((void *) NULL); /* S005 */
    } else
        return (d);
}

#ifdef GRAFDEBUG

/* Print all data in grafData structure */
DumpOneData (d)
    grafData *d;
{
    memList *m;                 /* S014 */
    portList *p;                /* S014 */
    intList *i;
    stringList *s;
    functionList *f;

    Eprint ("Memory:\n");
    for (m=d->memory; m; m=m->next) /* S014 */
        Eprint ("%s, 0x%x , 0x%x, 0x%x\n", /* S014 */
                m->name, m->base, m->size, m->mapped); /* S014 */

    Eprint ("Ports:\n");
    for (p=d->ports; p; p=p->next) /* S014 */
        Eprint ("0x%x , 0x%x\n", /* S014 */
                p->base, p->count); /* S014 */

    Eprint ("Ints:\n");
    for (i=d->integers; i; i=i->next)
        Eprint ("%s \t= 0x%x (%d)\n", i->id, i->val, i->val);

    Eprint ("Strings:\n");
    for (s=d->strings; s; s=s->next)
        Eprint ("%s \t= %s\n", s->id, s->val);

    Eprint ("Functions:\n");
    for (f=d->functions; f; f=f->next) {
        Eprint ("\n%s\n", f->id);
        DumpCode (f->code);
    }
}


#define	PRINTNR	if (code[codeIndex++]==G_NUMBER) \
	Eprint ("0x%x",code[codeIndex++]); \
	else Eprint ("r%d",code[codeIndex++]);
	

/* print out the code */
DumpCode (code)
    codeType *code;
{
    int codeIndex;

    for (codeIndex = 0; code[codeIndex] != OP_END; ) {
        switch (code[codeIndex++]) {
          case OP_ASSIGN:
            Eprint ("r%d = ", code[codeIndex++]);
            PRINTNR;
            Eprint("\n");
            break;
          case OP_AND:
            Eprint ("and r%d ", code[codeIndex++]);
            PRINTNR;
            Eprint ("\n");
            break;
          case OP_OR:
            Eprint ("or r%d ", code[codeIndex++]);
            PRINTNR;
            Eprint ("\n");
            break;
          case OP_XOR:
            Eprint ("xor r%d ", code[codeIndex++]);
            PRINTNR;
            Eprint ("\n");
            break;
          case OP_SHR:
            Eprint ("shr r%d ", code[codeIndex++]);
            PRINTNR;
            Eprint ("\n");
            break;
          case OP_SHL:
            Eprint ("shl r%d ", code[codeIndex++]);
            PRINTNR;
            Eprint ("\n");
            break;
          case OP_OUT:
            Eprint ("out ");
            PRINTNR;
            Eprint (",");
            PRINTNR;
            Eprint ("\n");
            break;
          case OP_OUTW:
            Eprint ("outw ");
            PRINTNR;
            Eprint (",");
            PRINTNR;
            Eprint ("\n");
            break;
          case OP_IN:
            Eprint ("in r%d ", code[codeIndex++]);
            PRINTNR;
            Eprint ("\n");
            break;
          case OP_NOT:
            Eprint ("not r%d\n", code[codeIndex]);
            codeIndex++;
            break;
          case OP_WAIT:
            Eprint ("wait 0x%x\n", code[codeIndex]);
            codeIndex++;
            break;
          case OP_BOUT:
            Eprint ("bout %d 0x%x 0x%x\n",
                    code[codeIndex],
                    code[codeIndex+1],
                    code[codeIndex+2]);
            codeIndex += 3;
            break;
          case OP_READB:
            Eprint ("readb r%d ", code[codeIndex++]);
            PRINTNR;
            Eprint ("\n");
            break;
          case OP_WRITEB:
            Eprint ("writeb ");
            PRINTNR;
            Eprint (",");
            PRINTNR;
            Eprint ("\n");
            break;
          case OP_READW:
            Eprint ("readw r%d ", code[codeIndex++]);
            PRINTNR;
            Eprint ("\n");
            break;
          case OP_WRITEW:
            Eprint ("writew ");
            PRINTNR;
            Eprint (",");
            PRINTNR;
            Eprint ("\n");
            break;
          case OP_READDW:
            Eprint ("readdw r%d ", code[codeIndex++]);
            PRINTNR;
            Eprint ("\n");
            break;
          case OP_WRITEDW:
            Eprint ("writedw ");
            PRINTNR;
            Eprint (",");
            PRINTNR;
            Eprint ("\n");
            break;
          case OP_INT10:        /* vvv S014 vvv */
            Eprint ("int10 r%d ", code[codeIndex++]);
            Eprint ("%d\n", code[codeIndex++]);
            break;              /* ^^^ S014 ^^^ */
          case OP_CALLROM:      /* vvv S018 vvv */
            Eprint ("callrom ");
            PRINTNR;
            Eprint (":");
            PRINTNR;
            Eprint (",");
            Eprint ("r%d ", code[codeIndex++]);
            Eprint ("%d\n", code[codeIndex++]);
            break;              /* ^^^ S018 ^^^ */
          default:
            Eprint ("illegal opcode\n");
            break;
        }
    }
}

#endif /* GRAFDEBUG */

/* vvv S014 vvv */
static void badGPtr(op)
    codeType op;
{
    Eprint("Operation unsupported by grafRunCode : opcode = %d\n", op);
    Eprint("    Use grafRunFunction instead.\n");
}

static void badAddr(addr, op)
    int addr;
    codeType op;
{
    Eprint("Bad read/write address : opcode = %d address = 0x%x\n",
           op, addr);
}

#define	CHECK_GPTR(g, op) \
		if ((g) == (grafData *) NULL) { \
		    badGPtr(op); \
		    continue; \
		}

#define CHECK_ADDR(vmem, pmem, op) \
		if ((vmem) == 0) { \
		    badAddr((pmem), (op)); \
		    continue; \
		}
		                                /* S020 */	
#define CHECK_ADDR_C(vmem, pmem, op) \
		if ((vmem) == 0) { \
		    badAddr((pmem), (op)); \
		    return; \
		}
		

#define	GETNUM()	(*cp++)
#define	GETNR()		(*cp++ == G_NUMBER ? *cp++ : reg[*cp++])

static int _grafCallRom(grafData *, int, int, int *, int);

/* execute code with environment (grafData) */			/* S014 */
void grafRunFunction (g, code, result, a0, a1, a2 ,a3, a4, a5, a6, a7, a8, a9)
    grafData *g;
    codePnt code;                                              /* S020 */
    int *result, a0, a1, a2 ,a3, a4, a5, a6, a7, a8, a9;
{
    cur_g = g;                  /* vvv S020 vvv */
    if( g->cType == COMPILED )
    {
        int res;
        res = ( code.function )(
                                a0, a1, a2 ,a3, a4, a5, a6, a7, a8, a9 );
        if( result != NULL )
            *result = res;
    }
    else
    {                                                      
        register codeType *cp = code.data; /* ^^^ S020 ^^^ */
        register int i, n;
        int port, port2, pmem, vmem;
        codeType op;
        int reg[NUMBER_REG];
      
      
        /* S008 */
        reg[0] = a0;
        reg[1] = a1;
        reg[2] = a2;
        reg[3] = a3;
        reg[4] = a4;
        reg[5] = a5;
        reg[6] = a6;
        reg[7] = a7;
        reg[8] = a8;
        reg[9] = a9;
      
        while ((op = *cp++) != OP_END) {
            switch (op) {
              case OP_ASSIGN:
                i = GETNUM();
                n = GETNR();
                reg[i] = n;
                continue;
              case OP_AND:
                i = GETNUM();
                n = GETNR();
                reg[i] &= n;
                continue;
              case OP_OR:
                i = GETNUM();
                n = GETNR();
                reg[i] |= n;
                continue;
              case OP_XOR:
                i = GETNUM();
                n = GETNR();
                reg[i] ^= n;
                continue;
              case OP_SHR:
                i = GETNUM();
                n = GETNR();
                reg[i] >>= n;
                continue;
              case OP_SHL:
                i = GETNUM();
                n = GETNR();
                reg[i] <<= n;
                continue;
              case OP_OUT:
                port = GETNR();
                n = GETNR();
                outp(port, n);
                continue;
              case OP_OUTW:
                port = GETNR();
                n = GETNR();
                outpw(port, n);
                continue;
              case OP_IN:
                i = GETNUM();
                port = GETNR();
                reg[i] = inp(port);
                continue;
              case OP_NOT:
                i = GETNUM();
                reg[i] = !reg[i];
                continue;
              case OP_WAIT:
                n = GETNUM();
                nap((n + 999) / 1000); /* S012 */
                continue;
              case OP_BOUT:
                n = GETNUM();
                port = GETNUM();
                port2 = GETNUM();
                for (i = 0; i < n; i++) {
                    outp(port,  i);
                    outp(port2, reg[i]);
                }
                continue;
              case OP_READB:
                i = GETNUM();
                pmem = GETNR();
                CHECK_GPTR(g, op);
                vmem = grafPmemVmem(g, pmem);
                CHECK_ADDR(vmem, pmem, op);
                reg[i] = *((unsigned char *) vmem);
                continue;
              case OP_WRITEB:
                pmem = GETNR();
                n = GETNR();
                CHECK_GPTR(g, op);
                vmem = grafPmemVmem(g, pmem);
                CHECK_ADDR(vmem, pmem, op);
                *((unsigned char *) vmem) = n;
                continue;
              case OP_READW:
                i = GETNUM();
                pmem = GETNR();
                CHECK_GPTR(g, op);
                vmem = grafPmemVmem(g, pmem);
                CHECK_ADDR(vmem, pmem, op);
                reg[i] = *((unsigned short *) vmem);
                continue;
              case OP_WRITEW:
                pmem = GETNR();
                n = GETNR();
                CHECK_GPTR(g, op);
                vmem = grafPmemVmem(g, pmem);
                CHECK_ADDR(vmem, pmem, op);
                *((unsigned short *) vmem) = n;
                continue;
              case OP_READDW:
                i = GETNUM();
                pmem = GETNR();
                CHECK_GPTR(g, op);
                vmem = grafPmemVmem(g, pmem);
                CHECK_ADDR(vmem, pmem, op);
                reg[i] = *((unsigned long *) vmem);
                continue;
              case OP_WRITEDW:
                pmem = GETNR();
                n = GETNR();
                CHECK_GPTR(g, op);
                vmem = grafPmemVmem(g, pmem);
                CHECK_ADDR(vmem, pmem, op);
                *((unsigned long *) vmem) = n;
                continue;
              case OP_INT10:    /* vvv S014 vvv */
                i = GETNUM();
                n = GETNUM();
                CHECK_GPTR(g, op);
                grafVBRun(g, &reg[i], n);
                continue;       /* ^^^ S014 ^^^ */
              case OP_CALLROM:  /* vvv S018 vvv */
                {
                    int seg = GETNR();
                    int off = GETNR();
                    i = GETNUM();
                    n = GETNUM();
                    CHECK_GPTR(g, op);
                    _grafCallRom(g, seg, off, &reg[i], n);
                }
                continue;       /* ^^^ S018 ^^^ */
              default:
                Eprint("grafRunFunction: illegal opcode %d\n", op);
            }
        }
      
        if (result != NULL)
            *result = reg[0];
    } /* end INTERPRET */
}


/* put a string in lower case - probably should be more efficient */
void strlwr (s)
    register char *s;
{
    register int c;

    while (c = *s){
        if ((c >='A') && (c <= 'Z'))
            *s = c + ('a' - 'A');
        s++;
    }
}

/* return a pointer to the full mode */
char *grafGetFullMode (modename, ttyname)
    char *modename, *ttyname;
{
    int i, c, ttylength;
    char tmp[STRLEN], s1[STRLEN], s2[STRLEN], s3[STRLEN];
    char s4[STRLEN], fname[STRLEN];
    char *vendor=NULL, *model=NULL, *class=NULL, *mode=NULL, *p;

    graferrno = GEOK;

    /*
     * first we have to figure out the vendor, model, class, and mode.
     * These may be given in the mode string. If thery are they have
     * precedence over other sources of this information.
     */

    if ((modename !=NULL) && (modename[0] != '\0')) {
        c = sscanf (modename, "%[^.].%[^.].%[^.].%s", s1, s2, s3, s4);

        /* the format is this string should be:
           [[[[vendor.]model.]class.]mode]
           */
        switch (c) {
          case 4:
            vendor = strdup (s1);
            model = strdup (s2);
            class = strdup (s3);
            mode = strdup (s4);
            break;
          case 3:
            model = strdup (s1);
            class = strdup (s2);
            mode = strdup (s3);
            break;
          case 2:
            class = strdup (s1);
            mode = strdup (s2);
            break;
          case 1:
            mode = strdup (s1);
            break;
          default:
            graferrno = GEMODESTRING;
            break;
        }
    }

    if (graferrno != GEOK)
        return (NULL);

    /* If we don't have everything yet, fill in the rest */

    if (vendor == NULL) {
        int found = 0;
        FILE *fp;
        /*
         * if ttyname is given, find the info from the grafdev file
         */
        if ((ttyname != NULL)  && (ttyname[0] != '\0')) {

            ttylength = strlen (ttyname);
            sprintf (fname, "%s/%s", grafdir, grafdev);
            if ((fp = fopen (fname, "r"))) {
                while (!found && fgets (tmp, STRLEN, fp)) {
                    tmp[ttylength] = 0;
                    if (strcmp (tmp, ttyname) == 0) {
                        found = 1;

                        p = &tmp[ttylength+1];

                        if (sscanf (p, "%[^.].%[^.].%[^.].%s", s1,
                                    s2,s3, s4) != 4) {
                            graferrno = GEFORMATDEV;
                            fclose (fp);
                            return (NULL);
                        }

                        if (!vendor)
                            vendor = strdup (s1);
                        if (!model)
                            model = strdup (s2);
                        if (!class)
                            class = strdup (s3);
                        if (!mode)
                            mode = strdup (s4);
                    } /* if (strcmp ... */
                } /* while */
                fclose (fp);
            } /* if (fopen... */
        } /* if ttyname */

        /* resort to the grafinfo.def file */
        if  (!found) {
            sprintf (fname, "%s/%s", grafdir, grafdef);
            if ((fp = fopen (fname, "r"))) {

                if (fscanf (fp, "%[^.].%[^.].%[^.].%s",
                            s1, s2, s3, s4) != 4) {
                    graferrno = GEFORMATDEF;
                    fclose (fp);
                    return (NULL);
                }

                if (!vendor)
                    vendor = strdup (s1);
                if (!model)
                    model = strdup (s2);
                if (!class)
                    class = strdup (s3);
                if (!mode)
                    mode = strdup (s4);

                fclose (fp);
            }
        }

        /* If we still haven't got it */
        if (vendor == NULL) {
            graferrno = GENODEVDEF;
            return (NULL);
        }

    } /* if (vendor == NULL) */

    /* now we should have all the strings filled in properly */

    /* make everything lower case */

    strlwr (vendor);
    strlwr (model);
    strlwr (class);
    strlwr (mode);

    /* generate the full mode */
    sprintf (tmp, "%s.%s.%s.%s", vendor, model, class, mode);

    p = strdup(tmp);
    if (p==NULL)
        graferrno=GEALLOC;

    /* S007 vvvvv */
    else
    {
        free ((void *)vendor);
        free ((void *)model);
        free ((void *)class);
        free ((void *)mode);
    }
    /* S007 ^^^^ */

    return (p);
}

/*
returns a pointer to a string with a valid file name for a grafinfo file
mode must be full
*/

char *grafGetName (modename)
    char *modename;
{
    char s1[STRLEN], s2[STRLEN], tmp[STRLEN], *p;

    if (sscanf(modename, "%[^.].%[^.].*%[^.].%*s", s1, s2) !=2 ) { /*S007*/
        graferrno=GEBADMODE;
        return (NULL);
    }

    /* generate the filename */

    sprintf (tmp, "%s/%s/%s.xgi", grafdir, s1, s2);

    p = strdup(tmp);
    if (p==NULL)
        graferrno=GEALLOC;
    return (p);
}
                                       /* vvv S020 vvv */
/* 
returns a pointer to a string with a valid file name for a loadable grafinfo 
file ( compiled and linked C-code )
mode must be full
*/

char *grafGetCName (modename)
    char *modename;
{
    char s1[STRLEN], s2[STRLEN], tmp[STRLEN], *p;

    if (sscanf(modename, "%[^.].%[^.].*%[^.].%*s", s1, s2) !=2 ) { 
        graferrno=GEBADMODE;
        return (NULL);
    }

    /* generate the filename */

    sprintf (tmp, "%s/%s/X%s.o", grafdir, s1, s2);

    p = strdup(tmp);
    if (p==NULL)
        graferrno=GEALLOC;
    return (p);
}
                                                       /* ^^^ S020 ^^^ */

/*
returns a SUCCESS or FAILURE and pointer to a grafData structure
mode is full
*/

int grafParseFile (filename, modename, g)
    char *filename, *modename;
    grafData *g;
{
    char vendor[STRLEN], model[STRLEN], class[STRLEN], mode[STRLEN];
    char vendorS[STRLEN], modelS[STRLEN], classS[STRLEN], modeS[STRLEN];
    char vendorSS[STRLEN], modelSS[STRLEN], classSS[STRLEN], modeSS[STRLEN];
    FILE *fp;
    int found, t;

    char              * cFilename; /* vvv S020 vvv */
    cFunctionStruct   * loadPoint;
    functionList      * prev;
    functionList      * next;
    int		  fd;           /* ^^^ S020 ^^^ */

    graferrno = GEOK;

    /* extract vendor, model, class, mode */
    if (sscanf (modename, "%[^.].%[^.].%[^.].%s",
		vendor, model, class, mode) != 4) {
        graferrno=GEBADMODE;
        return (FAILURE);
    }

    /* open file */
    if (!(fp=fopen(filename, "r"))) {
        graferrno = GEOPENFILE;
        return (FAILURE);
    }

    /* seek to the correct place in file */
    yyin = fp;
    for (found = 0;t=yylex();) {
        if (t!=VENDOR)
            continue;
        strcpy (vendorS, yylval.string);
        if (yylex () != STRING) {
            graferrno = GEBADPARSE;
            break;
        }
        strcpy (vendorSS, yylval.string);

        if (yylex()!=MODEL) {
            graferrno = GEBADPARSE; /* S007 */
            break;
        }
        strcpy (modelS, yylval.string);
        if (yylex () != STRING) {
            graferrno = GEBADPARSE;
            break;
        }
        strcpy (modelSS, yylval.string);

        if (yylex()!=CLASS) {
            graferrno = GEBADPARSE; /* S007 */
            break;
        }
        strcpy (classS, yylval.string);
        if (yylex () != STRING) {
            graferrno = GEBADPARSE;
            break;
        }
        strcpy (classSS, yylval.string);

        if (yylex()!=MODE) {
            graferrno = GEBADPARSE; /* S007 */
            break;
        }
        strcpy (modeS, yylval.string);
        if (yylex () != STRING) {
            graferrno = GEBADPARSE;
            break;
        }
        strcpy (modeSS, yylval.string);

        if ( (strcasecmp (vendor, vendorS) == 0) &&
            (strcasecmp (model, modelS) == 0) &&
            (strcasecmp (class, classS) == 0) &&
            (strcasecmp (mode, modeS) == 0)) {
            found = 1;
            break;
        }
    }

    if (!found) {
        fclose (fp);
        if (graferrno==GEOK)
            graferrno = GENOMODE;
        return (FAILURE);
    }

    /* set up part of the grafData structure */
    g->memory = (memList *) NULL; /* S014 */
    g->ports = (portList *) NULL; /* S014 */
    g->integers = (intList *) NULL;
    g->strings = (stringList *) NULL;
    g->functions = (functionList *) NULL;
    /* vvv S020 vvv */
    /*
     *  Lets see whether there is also a .c File for this mode
     *  If it loads successfully we set the ->cFunctions pointer in
     *  the grafData structure.
     *
     *  Our parser does not understand C-code
     *  Therefore if it sees the cFunctions pointer set it will skip
     *  all PROCEDURE clauses 
     */

    cFilename = grafGetCName( modename );
    if( (fd = open( cFilename, O_RDONLY )) >= 0)
    {
        loadPoint = ( cFunctionStruct * )dyddxload( fd, &grafLdInfo );
        NOTICE2(DEBUG_INIT,
                "Compiled grafinfo file \"%s\" loaded,\n  loadPoint 0x%x\n",
                cFilename, loadPoint );
        (void) close( fd );
    }
    else
    {
        NOTICE2(DEBUG_INIT,
                "Interpreted grafinfo file \"%s\" 0x%x\n",
                filename, 0 );
        loadPoint = NULL;
    }

    if( loadPoint )    
    {
        g->cType = COMPILED;

        while( loadPoint->mode != NULL ) 
        {
            if( strcmp( modename, loadPoint->mode ) == 0)
            {
                next = ( functionList *) GrafMalloc( sizeof( functionList));
                next->next          = NULL;
                next->id            = loadPoint->procedureName;
                next->code.function = loadPoint->functionPnt;

                if( g->functions == NULL )  g->functions = next;
                else                        prev->next   = next;
                prev = next;
            }
            loadPoint += 1;
        }
    }
    else
        g->cType = INTERPRET;
    /* ^^^^ S020 ^^^^ */
    /* alloc some memory for parser */
    grafCode = (codeType *) GrafMalloc (CODESIZE * sizeof(codeType));
    if (grafCode == (codeType *) NULL)
        return (FAILURE);

    parsit=g;

    /* call yacc code to parse this */
    yyparse ();

    free((void *)grafCode);

    fclose (fp);

    /* fill in vendor, model, class, mode  fields and their descriptions */

    /* S007 S014 */
    if (!AddString (g, "VENDOR",	   vendorS)	||
        !AddString (g, "vendorString", vendorSS)	||
        !AddString (g, "MODEL",	   modelS)	||
        !AddString (g, "modelString",  modelSS)	||
        !AddString (g, "CLASS",	   classS)	||
        !AddString (g, "classString",  classSS)	||
        !AddString (g, "MODE",	   modeS)	||
        !AddString (g, "modeString",   modeSS)	||
        (graferrno != GEOK) )
        return (FAILURE);

    if (!grafMemManage (g) )
        return (FAILURE);

    return (SUCCESS);
}

/* free all data associated with a mode */
int grafFreeMode (g)
    grafData *g;
{
    memList *m, *mn;            /* S014 */
    portList *p, *pn;           /* S014 */
    intList *i, *in;
    stringList *s, *sn;
    functionList *f, *fn;

    /* vvv S014 vvv */
    if (g->memory)
        for (m=g->memory, mn=m->next; m; m=mn, mn=m->next) {
            free ((void *)m->name);
            free ((void *)m);
        }

    if (g->ports)
        for (p=g->ports, pn=p->next; p; p=pn, pn=p->next) {
            free ((void *)p);
        }
    /* ^^^ S014 ^^^ */

    if (g->integers)
        for (i=g->integers, in=i->next; i; i=in, in=i->next) {
            free ((void *)i->id);
            free ((void *)i);
        }

    if (g->strings)
        for (s=g->strings, sn=s->next; s; s=sn, sn=s->next) {
            free ((void *)s->id);
            free ((void *)s->val);
            free ((void *)s);
        }

    if (g->functions)
        for (f=g->functions, fn=f->next; f; f=fn, fn=f->next) {
            if(g->cType == INTERPRET ) { /* S020 */
                free ((void *)f->id);
                /*free (f->code);*/
            }
            free ((void *)f);
        }

    if (g->cType == COMPILED )  /* S020 */
        dyddxunload( &grafLdInfo );


    g->memory = (memList *) NULL; /* S014 */
    g->ports = (portList *) NULL; /* S014 */
    g->integers = (intList *) NULL;
    g->strings = (stringList *) NULL;
    g->functions = (functionList *) NULL;

    return (SUCCESS);
}
	
/*
 returns SUCCESS or FAILURE if identifier was found
 identifier value in val
*/

int grafGetInt (g, id, val)
    grafData *g;
    char *id;
    int *val;
{
    intList *i;
    for (i=g->integers; i ; i=i->next)
        if (strcmp (id, i->id) == 0) {
            *val = i->val;
            return (SUCCESS);
        }
    return (FAILURE);
}

/*
 returns SUCCESS or FAILURE if identifier was found
 identifier value in val
 needs to be duped if g is to be freed up later on
*/

int grafGetString (g, id, val)
    grafData *g;
    char *id;
    char **val;
{
    stringList *s;
    for (s=g->strings; s ; s=s->next)
        if (strcmp (id, s->id) == 0) {
            *val = s->val;
            return (SUCCESS);
        }
    return (FAILURE);
}

/*
 S009
 returns SUCCESS or FAILURE if function was found
 identifier value in val
*/


int grafGetFunction (g, id, code)
    grafData *g;
    char *id;
    codePnt *code;                                          /* S020  */
{
    functionList *f;
    cur_g = g;                  /* save it for grafRunCode */
    for (f=g->functions; f ; f=f->next)
        if (strcmp (id, f->id) == 0) {
            /* S008 */
            *code = f->code;
            return (SUCCESS);
        }
    return (FAILURE);
}

							/* vvv S014 vvv */
/*
 backwards compatibility
 execute code without environment (grafData)
*/

void grafRunCode (code, result, a0, a1, a2 ,a3, a4, a5, a6, a7, a8, a9)
    codePnt code;
    int *result, a0, a1, a2 ,a3, a4, a5, a6, a7, a8, a9;
{
    grafRunFunction ( cur_g, code, /*  S020 */
                     result, a0, a1, a2 ,a3, a4, a5, a6, a7, a8, a9);
}
							/* ^^^ S014 ^^^ */

/*
 returns SUCCESS or FAILURE if function was found
 execs function
*/

int grafExec (g, function, result, a0, a1, a2 ,a3, a4, a5, a6, a7, a8, a9)
    grafData *g;
    char *function;
    int *result, a0, a1, a2 ,a3, a4, a5, a6, a7, a8, a9;
{
    functionList *f;

    for (f=g->functions; f ; f=f->next)
        if (strcmp (function, f->id) == 0) {
            /* S008  S014 */
            grafRunFunction (g, f->code,
                             result, a0, a1, a2 ,a3, a4, a5, a6, a7, a8, a9);
            return (SUCCESS);
        }
    return (FAILURE);
}

/*
   returns G_FUNCTION if query is a function
   returns G_INTEGER or G_STRING if query was declared in InitGraphics
   returns FAILURE otherwise
   */
int grafQuery (g, query)
    grafData *g;
    char *query;
{
    intList *i;
    stringList *s;
    functionList *f;

    for (f=g->functions; f ; f=f->next)
        if (strcmp (query, f->id) == 0)
            return (G_FUNCTION);

    for (i=g->integers; i ; i=i->next)
        if (strcmp (query, i->id) == 0)
            return (G_INTEGER);

    for (s=g->strings; s ; s=s->next)
        if (strcmp (query, s->id) == 0)
            return (G_STRING);

    return (FAILURE);
}

/* copy an array of codeTypes into another - like bcopy. If the code
   data type changes, this should also change
   */
static void codeCopy (src, dst, n)
    codeType *src, *dst;
    int n;
{
    int i;
    for (i=0; i<n; i++)
        dst[i] = src[i];
}

int AddFunction (g, functionName, functionCode, codeSize)
    grafData *g;
    char *functionName;
    codeType *functionCode;
    int codeSize;
{
    functionList *t;

    t = (functionList *) GrafMalloc (sizeof (functionList));
    if (t==(functionList *) NULL)
        return (FAILURE);

    if ((t->id=strdup(functionName)) == NULL) {
        graferrno = GEALLOC;
        return (FAILURE);
    }
    /* S020 */
    t->code.data = (codeType *) GrafMalloc (sizeof (codeType) * codeSize);
    if (t->code.data == (codeType *) NULL)
        return (FAILURE);
	
    codeCopy (functionCode, t->code, codeSize );

    /* link it in */
    t->next = (functionList *) g->functions;
    g->functions = t;

    return (SUCCESS);
}

int AddString (g, id, val)
    grafData *g;
    char *id, *val;
{
    stringList *t;

    t = (stringList *) GrafMalloc (sizeof (stringList));
    if (t==(stringList *) NULL)
        return (FAILURE);

    if ((t->id=strdup(id)) == NULL) {
        graferrno = GEALLOC;
        return (FAILURE);
    }

    if ((t->val=strdup(val)) == NULL) {
        graferrno = GEALLOC;
        return (FAILURE);
    }

    /* link it in */
    t->next = (stringList *) g->strings;
    g->strings = t;

    return (SUCCESS);
}

int AddInt (g, id, val)
    grafData *g;
    char *id;
    int val;
{
    intList *t;

    t = (intList *) GrafMalloc (sizeof (intList));
    if (t==(intList *) NULL)
        return (FAILURE);

    if ((t->id=strdup(id)) == NULL) {
        graferrno = GEALLOC;
        return (FAILURE);
    }

    t->val=val;

    /* link it in */
    t->next = (intList *) g->integers;
    g->integers = t;

    return (SUCCESS);
}

/* vvv S014 vvv */
int AddMem (g, name, base, size)
    grafData *g;
    char *name;
    int base, size;
{
    memList *t;

    t = (memList *) GrafMalloc (sizeof (memList));
    if (t==(memList *) NULL)
        return (FAILURE);

    if ((t->name=strdup(name)) == NULL) {
        graferrno = GEALLOC;
        return (FAILURE);
    }

    t->base=base;
    t->size=size;

    /* link it in */
    t->next = (memList *) g->memory;
    g->memory = t;

    return (SUCCESS);
}

int AddPort (g, base, count)
    grafData *g;
    int base, count;
{
    portList *t;

    t = (portList *) GrafMalloc (sizeof (portList));
    if (t==(portList *) NULL)
        return (FAILURE);

    t->base=base;
    t->count=count;

    /* link it in */
    t->next = (portList *) g->ports;
    g->ports = t;

    return (SUCCESS);
}

int AddPortGroup (g, group)
    grafData *g;
    char *group;
{
    if (strcmp(group, "VGA") == 0)
        return (AddPort(g, 0x3B0, 0x30));

    if (strcmp(group, "EFF") == 0) {
        register int port = 0x02E8;
        do {                    /* vvv S018 vvv */
            if (!AddPort(g, port, 8))
                return (FAILURE);
        } while ((port += 0x0400) < 0x10000); /* ^^^ S018 ^^^ */
        return (SUCCESS);
    }

    return (FAILURE);
}
							/* ^^^ S014 ^^^ */

/* return a pointer to a list of , separated modes */
char *grafGetTtyMode (ttyname)
    char *ttyname;
{
    int i, c, ttylength;
    char tmp[LSTRLEN], fname[STRLEN], mode[LSTRLEN], *p; /* S019 */
    int found = 0;
    FILE *fp;
    graferrno = GEOK;

    *mode='\0';

    if ((ttyname != NULL)  && (ttyname[0] != '\0')) {
        ttylength = strlen (ttyname);
        sprintf (fname, "%s/%s", grafdir, grafdev);
        if ((fp = fopen (fname, "r"))) {
            while (fgets (tmp, LSTRLEN, fp)) { /* S019 */
                tmp[ttylength] = 0;
                if (strcmp (tmp, ttyname) == 0) {
                    fclose (fp);
                    strcpy(mode, &tmp[ttylength+1]);
                } /* if (strcmp ... */
            } /* while */
            fclose (fp);
        } /* if (fopen... */
    } /* if ttyname */

    if(*mode == '\0') {
        /* resort to the grafinfo.def file */
        sprintf (fname, "%s/%s", grafdir, grafdef);
        if ((fp = fopen (fname, "r"))) {
            fgets (mode, LSTRLEN, fp);
            fclose (fp);
        }
    }

    if(*mode != '\0')
    {
        /* Clean the garbage off the end of the string */
        for (i = strlen(mode) - 1; isspace(mode[i]); i--)
            mode[i]='\0';

        p = strdup (mode);
        if (p==NULL)
            graferrno = GEALLOC;
        return (p);
    }

    graferrno = GEBADMODE;
	
    return NULL;
}

/*  in MemConMap.c					vvv S028	*/
void *
memconmap( void *addr, size_t len, int prot, int flags, int fd,
          off_t off, int ioctlFd, int DoConMemMap, int DoZero );
/*							^^^ S028	*/

/* vvv S014 vvv */
int grafMemManage (g)
    grafData *g;
{
    memList *m;
    char *class, memclass[BUFSIZ];
#if defined(usl)
    int PmemFd;
    int videoFd;			/*	S027	*/
    int cmmRet;				/*	S027	*/
#endif

    graferrno=GEOK;

    if (!grafGetString(g, "CLASS", &class)) {
        Eprint ("graf: Missing CLASS in grafinfo file\n");
        graferrno = GENOCLASS;
        return (FAILURE);
    }

    for (m = g->memory; m; m = m->next) {
        sprintf(memclass, "%s%s", class, m->name);
#if defined(usl)
	PmemFd = open("/dev/pmem",O_RDWR);
	if (PmemFd < 0) {
            ErrorF("Cannot open /dev/pmem\n");
            return(FAILURE);
	}

	videoFd = open("/dev/video", O_RDWR);	/*	S027 vvv	*/
	if (videoFd < 0) {
	    close(PmemFd);
            ErrorF("Cannot open %s\n", "/dev/video" );
            return(FAILURE);
	}					/*	S027 ^^^	*/

	if ( m->base < 0xe0000 ) {

    m->mapped = (unsigned int) memconmap( (void *) m->base, m->size,
	PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, PmemFd,
	m->base, videoFd, 1, 0 );	/*	S028	*/

	} else {

    m->mapped = (unsigned int) memconmap( (void *) m->base, m->size,
	PROT_READ|PROT_WRITE, MAP_SHARED, PmemFd,
	m->base, videoFd, 1, 1 );	/*	S028	*/

	}
	if ((caddr_t)m->mapped == (caddr_t)-1) {
            Eprint("graf: Cannot mmap video memory at %#x for length %#x\n", m->base, m->size );
            graferrno = GEMAPCLASS;
	    close(videoFd);	/*	S027	*/
            close(PmemFd);
            return(FAILURE);
	}
	close(videoFd);	/*	S027	*/
	close(PmemFd);
#else
        if ((m->mapped = ioctl(1, MAP_CLASS, memclass)) == -1) {
            Eprint("graf: %s MAP_CLASS failed\n", memclass);

            graferrno = GEMAPCLASS;
            return (FAILURE);
        }
#endif /*	usl	*/
    }

    return (SUCCESS);
}

/* map a physical memory address to a virtual memory address */
int grafPmemVmem (g, pmem)
    grafData *g;
    unsigned int pmem;
{
    memList *m;
    unsigned int memoff;

    for (m = g->memory; m; m = m->next)
	if ( (pmem >= m->base) && ((memoff = pmem - m->base) < m->size) )
	    return (m->mapped + memoff);

    return (0);
}

/* return info about memory by name */
int grafGetMemInfo(g, name, pbase, psize, pmapped)
    grafData *g;
    char *name;
    unsigned int *pbase, *psize, *pmapped;
{
    memList *m;

    if (name == NULL)
        name = "";

    for (m = g->memory; m; m = m->next) {
        if (strcmp(name, m->name) == 0) {
            if (pbase)	*pbase	 = m->base;
            if (psize)	*psize	 = m->size;
            if (pmapped)	*pmapped = m->mapped;
            return (SUCCESS);
        }
    }

    return (FAILURE);
}


#if defined(usl)

#undef USE_VBIOS_DAEMON
#ifdef USE_VM86_DAEMON
#define vm86Init vm86DaemonInit
#define vm86Int10 vm86DaemonInt10
#define vm86CallRom vm86DaemonCallRom
#endif

#else  /* usl */

/* vvv S018 vvv */
/* X-server must do vbios stuff in a daemon */
#ifdef XSERVER
#define	USE_VBIOS_DAEMON	1
#endif

#endif	/*	usl	*/

/* if we aren't using a daemon, call v86bios directly */
#ifndef USE_VBIOS_DAEMON
#define	vbInit		VBiosInit
#define	vbMemMap(b,s)	V86MemMap(b,b,s)
#define	vbIOEnable	V86IOEnable
#define	vbInt10		VBiosINT10
#define	vbCallRom	VBiosCall
#endif

/* setup for video bios/rom */

static int grafVBInitDone = 0;	/* not private to avoid call to grafVBInit */

static
int grafVBInit(g, seg)
     grafData *g;
     int	 seg;
{
  int opts = 0;
  memList  *m;
  portList *p;
#if defined(usl)
  int PmemFd;
  extern int interpInit(int);
  int videoFd;			/*	S027	*/
  int cmmRet;				/*	S027	*/
#else
  char *memfile = NULL;
#endif
    
  grafGetInt(g, "VBIOSOPTS", &opts);

#if defined(usl)

  if( grafVBInitDone )
    return 0;
  if (!interpInit(opts)) {
    Eprint ("grafVBInit failed:\n");
    return -1;
  }

  grafVBInitDone = 1;

  PmemFd = open("/dev/pmem",O_RDWR);
  if (PmemFd < 0) {
    ErrorF("Cannot open /dev/pmem\n");
    return(FAILURE);
  }

  videoFd = open("/dev/video", O_RDWR);	/*	S027 vvv	*/
  if (videoFd < 0) {
	close(PmemFd);
	ErrorF("Cannot open %s\n", "/dev/video" );
	return(FAILURE);
  }					/*	S027 ^^^	*/

  for (m = g->memory; m; m = m->next) {

    if ( ! m->mapped ) {
      if ( m->base < 0xe0000 ) {

    m->mapped = (unsigned int) memconmap( (void *) m->base, m->size,
	PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, PmemFd,
	m->base, videoFd, 1, 0 );	/*	S028	*/

      } else {

    m->mapped = (unsigned int) memconmap( (void *) m->base, m->size,
	PROT_READ|PROT_WRITE, MAP_SHARED, PmemFd,
	m->base, videoFd, 1, 1 );	/*	S028	*/

      }

      if ((caddr_t)m->mapped == (caddr_t)-1) {
        Eprint("grafVBInit: Cannot mmap video memory at %#x for length %#x\n",
               m->base, m->size );
        graferrno = GEMAPCLASS;
        close(videoFd);
        close(PmemFd);
        return(FAILURE);
      }
    }
  }
  close(videoFd);
  close(PmemFd);

  /* enable I/O to listed ports */		/*	S025	vvv	*/
  for (p = g->ports; p; p = p->next) {
    if( ! vbIOEnable(p->base, p->count) )
      return(FAILURE);
  }						/*	S025	^^^	*/

#ifdef V86BIOS
  if (!(opts & OPT_DISABLE_VM86))
    vm86Init();
#endif /* V86BIOS */

  return SUCCESS;

#else

  grafGetString(g, "VBMEMFILE", &memfile);

  /* init v86 mode and video bios */
  if (vbInit(opts, seg, memfile) < 0)
    return -1;

  /* map listed memory below 1 Meg */
  for (m = g->memory; m; m = m->next) {
    if (m->base >= 0x100000)
      continue;
    if (vbMemMap(m->base, m->size) < 0) {
      Eprint("Can't map memory at 0x%x for BIOS\n", m->base);
      return -1;
    }
  }

  /* enable I/O to listed ports */
  for (p = g->ports; p; p = p->next)
    vbIOEnable(p->base, p->count);

  grafVBInitDone = 1;
  return 0;

#endif /*	usl	*/

}

/* video bios execution */
int grafVBRun(g, preg, nreg)
     grafData *g;
     int	 *preg;
     int	 nreg;
{
  int	 r;

  if (!grafVBInitDone)
    if (grafVBInit(g, 0) < 0)
      return;

#ifdef V86BIOS
  if (vm86Int10(preg, nreg) >= 0)
    return;
#endif

  /* run the int10 request */
  if ((r = vbInt10(preg, nreg)) < 0)
    Eprint("BIOS execution error %d\n", r);

}

/* video rom call */
static int _grafCallRom(g, seg, off, preg, nreg)   /* S020 */
     grafData *g;
     int	 seg;
     int	 off;
     int	 *preg;
     int	 nreg;
{
  int	 r;

  if (!grafVBInitDone)
    if (grafVBInit(g, seg) < 0)
      return;

#ifdef V86BIOS
  if (vm86CallRom(seg, off, preg, nreg) >= 0)
    return;
#endif

  /* call into the rom */
  if ((r = vbCallRom(seg, off, preg, nreg)) < 0)
    Eprint("ROM call error %d\n", r);
}
						/* ^^^ S014 S018 ^^^ */

/* vvv S020  vvvv */
/*
 *     These functions will be called from compiled grafinfo files
 */
void
grafOut( port, n )
    unsigned int port, n;
{
    outp(port, n);
}
void
grafOutw( port, n )
    unsigned int port, n;
{
    outpw(port, n);
}
unsigned int
grafIn( port )
    unsigned int port;
{
    return (( unsigned int) inp(port));
}
unsigned int                                      /*  S021 */
grafInw( port )
    unsigned int port;
{
    return (( unsigned int) inpw(port));
}
void
grafWait( n )
    unsigned int n;
{
    nap((n + 999) / 1000);                               
}
void
grafBout( reg, n, port, port2)
    unsigned int reg[];
    unsigned int n,port,port2;
{
    int i;
    for (i = 0; i < n; i++) {
        outp(port,  i);
        outp(port2, reg[i]);
    }
}
unsigned int
grafReadb(  pmem )
    unsigned int  pmem;
{
    int vmem;
    vmem = grafPmemVmem(cur_g, pmem);
    CHECK_ADDR_C(vmem, pmem, 13);
    return (*((unsigned char *) vmem));
}
unsigned int
grafReadw(  pmem )
    unsigned int pmem;
{
    int vmem;
    vmem = grafPmemVmem(cur_g, pmem);
    CHECK_ADDR_C(vmem, pmem, 15);
    return *((unsigned short *) vmem);
}
unsigned int
grafReaddw(  pmem )
    unsigned int pmem;
{
    int vmem;
    vmem = grafPmemVmem(cur_g, pmem);
    CHECK_ADDR_C(vmem, pmem, 17);
    return *((unsigned long *) vmem);
}
void 
grafWriteb( pmem, n )
    unsigned int pmem,n;
{
    int vmem;
    vmem = grafPmemVmem(cur_g, pmem);
    CHECK_ADDR_C(vmem, pmem, 14);
    *((unsigned char *) vmem) = n;
}
void 
grafWritew( pmem, n )
    unsigned int pmem,n;
{
    int vmem;
    vmem = grafPmemVmem(cur_g, pmem);
    CHECK_ADDR_C(vmem, pmem, 16);
    *((unsigned short *) vmem) = n;
}
void 
grafWritedw( pmem, n )
    unsigned int pmem,n;
{
    int vmem;
    vmem = grafPmemVmem(cur_g, pmem);
    CHECK_ADDR_C(vmem, pmem, 18);
    *((unsigned long *) vmem) = n;
}
void
grafInt10( reg, n )
    unsigned int reg[], n;
{
    grafVBRun(cur_g, reg, n);
}

grafCallRom( seg, off, reg, n )
    unsigned int seg, off, reg[], n ;
{
    _grafCallRom(cur_g, (int)seg, (int)off, (int *)reg, (int)n);
}
                                                        /* ^^^ S020 ^^^ */
