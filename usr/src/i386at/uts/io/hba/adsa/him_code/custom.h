#ident	"@(#)kern-pdi:io/hba/adsa/him_code/custom.h	1.1"

UBYTE inp(UWORD);
#pragma aux inp =		         \
    "in al, dx"               \
    parm nomemory [dx]		   \
    modify exact [al] nomemory ;

UWORD inpw(UWORD);
#pragma aux inpw =		      \
    "in ax, dx"               \
    parm nomemory [dx]		   \
    modify exact [ax] nomemory ;

void outp(UWORD,UBYTE);
#pragma aux outp =		      \
    "out dx, al"              \
    parm nomemory [dx] [al]	\
    modify exact [] nomemory	;

void outpw(UWORD,UWORD);
#pragma aux outpw =		      \
    "out dx, ax"              \
    parm nomemory [dx] [ax]	\
    modify exact [] nomemory	;

void _disable(void);
#pragma aux _disable =		   \
    "cli"                     \
    parm nomemory		         \
    modify exact nomemory []	;

void _enable(void);
#pragma aux _enable =		   \
    "sti"                     \
    parm nomemory		         \
    modify exact nomemory []	;

unsigned int SaveAndDisable( void );
#pragma aux SaveAndDisable =	\
	"pushfd"		               \
	"cli"			               \
	"pop eax"		            \
	parm nomemory		         \
	modify exact nomemory [eax];

unsigned int SaveAndEnable( void );
#pragma aux SaveAndEnable =	\
	"pushfd"		               \
	"sti"			               \
	"pop eax"		            \
	parm nomemory		         \
	modify exact nomemory [eax];

void RestoreState( unsigned int );
#pragma aux RestoreState =	   \
	"push eax"		            \
	"popfd"			            \
	parm nomemory [eax]	      \
	modify exact nomemory [];

void _DEBUG(void);
#pragma aux _DEBUG =		      \
    "int 3"			            \
    parm nomemory		         \
    modify exact nomemory []	;
