/*
 *	@(#)pnp_private.h	7.1	10/22/97	12:29:05
 * File pnp_private.h for the Plug-and-Play driver
 *
 * @(#) pnp_private.h 65.4 97/07/11 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include "pnp_compat.h"		/* OSR5/UNIXWARE/GEMINI compatability */

#ifdef PNP_DEBUG
#   define STATIC
#   define PNP_DPRINT	cmn_err
#else
#   define STATIC	static
#   define PNP_DPRINT	0 || 
#endif

#ifndef NULL
#   define NULL	0
#endif /* NULL */

#define PNP_ADDRESS_PORT	(0x0279)
#define PNP_WRITE_DATA_PORT	(0x0a79)

#define PNP_MIN_READ_DATA_PORT	(0x0203)	/* LS 2 bits are always 11 */
#define PNP_MAX_READ_DATA_PORT	(0x03ff)	/* As large as 0x03ff */

/*
 * Shared between modules but not public to other drivers.  Card
 * driver writers should NOT expect to know PnP_devices at all. 
 * They should use the published interface.
 */

typedef struct
{
    u_long	vendor;		/* These are the */
    u_long	serial;		/*   first nine bytes */
    u_char	chkSum;		/*   in the card information */

    u_char	CSN;
} cardID_t;

typedef struct PnP_device PnP_device_t;
struct PnP_device
{
    cardID_t		id;
    int			unit;
    u_long		NodeSize;
    u_long		ResCnt;
    u_char		devCnt;
    PnP_device_t	*next;
};

#define DEV_FLG_IS_SET	0x00000001


extern const char	PnP_name[];
extern u_long		PnP_NumNodes;
extern u_long		PnP_NodeSize;
extern PnP_device_t	*PnP_devices;
extern int		PnP_Alive;
extern u_long		PnP_dataPort;

extern PNP_MUTEXLOCK_T	PnP_devices_lock;

ENTRY_RETVAL	PnPinit(void);
PnP_device_t	*PnP_FindUnit(const PnP_unitID_t *up);
u_char		PnP_readResByte(void);
void		PnP_writeResByte(u_char data);
void		PnP_writeCmd(PnP_Register_t regNum, u_char value);
u_char		PnP_readRegister(PnP_Register_t regNum);
void		PnP_Wake(u_char csn);
void		PnP_SetRunState(void);
u_char		PnP_ReadActive(const PnP_device_t *dp, u_char device);
int		PnP_WriteActive(const PnP_device_t *dp,
						u_char device, u_char active);

/*
 * These should be in some system header
 */

#ifndef UNIXWARE
void	putchar(int c);
int	bcmp(const void *a, const void *b, size_t bytes);
int	cmn_err(int severity, char *format, ...);
void	bzero(void *, size_t len);
int	bcopy(const void *src, void *dest, size_t len);
int	outb(int addr, int value);
int	inb(int addr);
int	suspend(unsigned int microsecs);
void	seterror(char err);
int	printcfg(const char *, u_int, u_int, int, int, const char *, ...);
int	kstrncmp(const char *s1, const char *s2, int len);
int	copyin(const void *src, void *dst, int cnt);
int	copyout(void *src, const void *dst, int cnt);
int	lockb5(struct lockb *lock_xxtab);
void	unlockb(struct lockb *lock_xxtab, int oldspl);
caddr_t	sptalloc(int pages, int mode, pfn_t base, int flag);
void	sptfree(caddr_t va, int npages, int freeflg);
#endif	/* !UNIXWARE */

extern struct bootmisc bootmisc;	/* io/bs.c */
extern struct seg_desc gdt[];		/* ml/tables2.c */
extern short gdtend;			/* ml/tables2.c */

