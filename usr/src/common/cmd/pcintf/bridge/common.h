#ident	"@(#)pcintf:bridge/common.h	1.2"
/*
**	@(#)common.h	1.2	2/20/92	09:46:19
**	Copyright 1991  Locus Computing Corporation
**
**	extern function declarations
*/

#include <sys/types.h>
#include <sys/stat.h>

#include "pci_proto.h"
#include "pci_types.h"

extern int	netSave			PROTO((void));
extern int	netUse			PROTO((int));
extern int	rcvPacket		PROTO((struct input *));
extern int	xmtPacket		PROTO((struct output *, struct ni2 *, int));
extern int	reXmt			PROTO((struct output *, int));
extern int	nls_init		PROTO((void));
extern void	byteorder_init		PROTO((void));
extern void	drain_tty		PROTO((int));
extern char	*strdup			PROTO((CONST char *));
extern int	get_interface_list	PROTO((char *));
extern int	input_swap		PROTO((struct input *, long));
extern void	output_swap		PROTO((struct output *, int));
extern char	*myhostname		PROTO((void));
extern char	*fnQualify		PROTO((char *, char *));
extern int	uMaxDescriptors		PROTO((void));
extern char	*memory			PROTO((int));
extern char	*morememory		PROTO((char *, int));
extern char	*savestr		PROTO((char *));
extern void	sig_catcher		PROTO((int));
extern int	rdtest			PROTO((unsigned short, unsigned short));
extern void	pckRD			PROTO((struct emhead *, unsigned char, unsigned short, unsigned short, unsigned char, unsigned char, unsigned char));
extern void	pckframe		PROTO((struct output *, int, int, unsigned char, int, int, int, int, int, int, int, int, int, int, struct stat *));
extern unsigned char	attribute	PROTO((struct stat *, char *));

#if !defined(RS232PCI)

extern int	rdsem_init	PROTO((void));
extern int	rdsem_open	PROTO((void));
extern int	rdsem_access	PROTO((int));
extern int	rdsem_relinq	PROTO((int));
extern void	rdsem_unlnk	PROTO((int));
extern shmid_t	rd_sdinit	PROTO((void));
extern char	*rd_sdenter	PROTO((shmid_t));
extern int	rd_sdleave	PROTO((char *));
extern shmid_t	rd_sdopen	PROTO((void));
extern void	rd_shmdel	PROTO((shmid_t));

#endif /* !RS232PCI */

#if defined(UDP42)

extern int	netOpen		PROTO((char *, int));
extern char	*nAddrFmt	PROTO((unsigned char *));
extern int	nAddrGet	PROTO((char *, char *));
extern void	netaddr		PROTO((netIntFace *, int *, int));

#endif	/* UDP42 */

#ifdef RS232PCI

extern int		unstuff			PROTO((char *, char *, int));
extern int		stuff			PROTO((char *, int));
extern unsigned short	chksum			PROTO((char *, int));

#	ifdef RS232_7BIT

extern int		convert_to_7_bits	PROTO((unsigned char *, int));
extern int		convert_from_7_bits	PROTO((unsigned char *, int));

#	endif /* RS232_7BIT */
#endif /* RS232PCI */
