#ident	"@(#)Space.c	2.1"
#ident	"$Header$"

#include	<sys/types.h>
#include	<sys/route.h>
#include	<config.h>

/*
 * global variables
 */
struct _Board_SRTrack_ boardRoute[ODISR_NBOARD];

int	odisr_nboard = ODISR_NBOARD;

/* 
 * the followings define route broadcast for each frame
 * SR_LIM_BRDCAST_MASK or SR_GEN_BRDCAST_MASK (All routes broadcast)
 */
uchar_t	unknowndestaddr = SR_GEN_BRDCAST_MASK;
uchar_t	broadcastframes = SR_GEN_BRDCAST_MASK;
uchar_t	multicastframes = SR_GEN_BRDCAST_MASK;
uchar_t	brdcast_response = SR_GEN_BRDCAST_MASK;

int	odisr_boardage = ODISR_BOARDAGE;
