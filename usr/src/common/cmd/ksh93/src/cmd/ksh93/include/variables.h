#ident	"@(#)ksh93:src/cmd/ksh93/include/variables.h	1.1"
#pragma prototyped

#ifndef SH_VALNOD

#include        <option.h>
#include        "FEATURE/options"
#include        "FEATURE/dynamic"

/* The following defines are coordinated with data in data/variables.c */

#define	PATHNOD		(sh.bltin_nodes)
#define PS1NOD		(sh.bltin_nodes+1)
#define PS2NOD		(sh.bltin_nodes+2)
#define IFSNOD		(sh.bltin_nodes+3)
#define PWDNOD		(sh.bltin_nodes+4)
#define HOME		(sh.bltin_nodes+5)
#define MAILNOD		(sh.bltin_nodes+6)
#define REPLYNOD	(sh.bltin_nodes+7)
#define SHELLNOD	(sh.bltin_nodes+8)
#define EDITNOD		(sh.bltin_nodes+9)
#define MCHKNOD		(sh.bltin_nodes+10)
#define RANDNOD		(sh.bltin_nodes+11)
#define ENVNOD		(sh.bltin_nodes+12)
#define HISTFILE	(sh.bltin_nodes+13)
#define HISTSIZE	(sh.bltin_nodes+14)
#define HISTEDIT	(sh.bltin_nodes+15)
#define HISTCUR		(sh.bltin_nodes+16)
#define FCEDNOD		(sh.bltin_nodes+17)
#define CDPNOD		(sh.bltin_nodes+18)
#define MAILPNOD	(sh.bltin_nodes+19)
#define PS3NOD		(sh.bltin_nodes+20)
#define OLDPWDNOD	(sh.bltin_nodes+21)
#define VISINOD		(sh.bltin_nodes+22)
#define COLUMNS		(sh.bltin_nodes+23)
#define LINES		(sh.bltin_nodes+24)
#define PPIDNOD		(sh.bltin_nodes+25)
#define L_ARGNOD	(sh.bltin_nodes+26)
#define TMOUTNOD	(sh.bltin_nodes+27)
#define SECONDS		(sh.bltin_nodes+28)
#define LINENO		(sh.bltin_nodes+29)
#define OPTARGNOD	(sh.bltin_nodes+30)
#define OPTINDNOD	(sh.bltin_nodes+31)
#define PS4NOD		(sh.bltin_nodes+32)
#define FPATHNOD	(sh.bltin_nodes+33)
#define LANGNOD		(sh.bltin_nodes+34)
#define LCALLNOD	(sh.bltin_nodes+35)
#define LCCOLLNOD	(sh.bltin_nodes+36)
#define LCTYPENOD	(sh.bltin_nodes+37)
#define LCMSGNOD	(sh.bltin_nodes+38)
#define LCNUMNOD	(sh.bltin_nodes+39)
#define FIGNORENOD	(sh.bltin_nodes+40)
#define DOTSHNOD	(sh.bltin_nodes+41)
#define ED_CHRNOD	(sh.bltin_nodes+42)
#define ED_COLNOD	(sh.bltin_nodes+43)
#define ED_TXTNOD	(sh.bltin_nodes+44)
#define ED_MODENOD	(sh.bltin_nodes+45)
#define SH_NAMENOD	(sh.bltin_nodes+46)
#define SH_SUBSCRNOD	(sh.bltin_nodes+47)
#define SH_VALNOD	(sh.bltin_nodes+48)
#define SH_VERSIONNOD	(sh.bltin_nodes+49)
#define SH_DOLLARNOD	(sh.bltin_nodes+50)
#ifdef SHOPT_FS_3D
#   define VPATHNOD	(sh.bltin_nodes+51)
#   define NFS_3D	1
#else
#   define NFS_3D	0
#endif /* SHOPT_FS_3D */
#ifdef SHOPT_VPIX
#   define DOSPATHNOD	(sh.bltin_nodes+51+NFS_3D)
#   define VPIXNOD	(sh.bltin_nodes+52+NFS_3D)
#   define NVPIX	(NFS_3D+2)
#else
#   define NVPIX	NFS_3D
#endif /* SHOPT_VPIX */
#ifdef SHOPT_MULTIBYTE
#   define CSWIDTHNOD 	(sh.bltin_nodes+51+NVPIX)
#   define NMULTI	NVPIX+1
#else
#   define NMULTI       NVPIX
#endif /* SHOPT_MULTIBYTE */
#ifdef apollo
#   define SYSTYPENOD	(sh.bltin_nodes+51+NMULTI)
#endif /* apollo */

#endif /* SH_VALNOD */
