#ifndef _IO_TARGET_VTOC_VTOCDEBUG_H
#define _IO_TARGET_VTOC_VTOCDEBUG_H

#ident	"@(#)kern-pdi:io/layer/vtoc/vtocdebug.h	1.3.1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef DEBUG

/*
 * DEBUG LEVELS.
 *
 * 00000000 - No debug.
 * 00000001 - Entry points.
 * 00000002 - Fdisk table.
 * 00000004 - open entry point flags.
 * 00000008 - Strategy Transaltion results.
 * 00000010 - Absolute blocks of fdisk/vtoc/divvy.
 * 00000020 - Open and close entry points
 * 00000040 - Config entry points.
 * 00000080 - Ioctls.
 */
#define DBFLAG0(val,string)		\
		if((val) & vtocDbFlag)	\
			cmn_err(CE_NOTE, string)
#define DBFLAG1(val,string,parm1)	\
		if((val) & vtocDbFlag) \
			cmn_err(CE_NOTE, string,parm1)
#define DBFLAG2(val,string,parm1,parm2)	\
		if((val) & vtocDbFlag) \
			cmn_err(CE_NOTE, string,parm1,parm2)
#define DBFLAG3(val,string,parm1,parm2,parm3)	\
		if((val) & vtocDbFlag) \
			cmn_err(CE_NOTE, string,parm1,parm2,parm3)
#define DBFLAG4(val,string,parm1,parm2,parm3,parm4)	\
		if((val) & vtocDbFlag) \
			cmn_err(CE_NOTE,string,parm1,parm2,parm3,parm4)
#define DBFLAG5(val,string,parm1,parm2,parm3,parm4,parm5)	\
		if((val) & vtocDbFlag) \
			cmn_err(CE_NOTE,string,parm1,parm2,parm3,parm4,parm5)
#define DBFLAG6(val,string,parm1,parm2,parm3,parm4,parm5,parm6)	\
		if((val) & vtocDbFlag) \
			cmn_err(CE_NOTE,string,parm1,parm2,parm3,parm4,parm5,parm6)
#else

#define DBFLAG0(val,string)
#define DBFLAG1(val,string,parm1)
#define DBFLAG2(val,string,parm1,parm2)
#define DBFLAG3(val,string,parm1,parm2,parm3)
#define DBFLAG4(val,string,parm1,parm2,parm3,parm4)
#define DBFLAG5(val,string,parm1,parm2,parm3,parm4,parm5)
#define DBFLAG6(val,string,parm1,parm2,parm3,parm4,parm5,parm6)

#endif

#if defined(__cplusplus)
        }
#endif

#endif /* _IO_TARGET_VTOC_VTOCDEBUG_H */
