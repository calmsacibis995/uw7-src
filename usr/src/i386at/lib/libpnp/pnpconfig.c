/*
*	@(#)pnpconfig.c	7.4	12/18/97	11:05:43
*/
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include "sys/pnp.h"
#include <limits.h>
#include "tcl.h"

#include "hw_eisa.h"

#define EISA_MB_ID	0xfffd9		/* f000:ffd9 "EISA" */
#define NO_EISA_ID	0xffffffff

#define	EISA_BASE	0x0c80		/* base address of EISA IDs */
#define	EISA_SLOTS	16		/* slots including MB */
#define EISA_NEXT	0x1000		/* Offset to next adapter */
#define	EISA_SLOT(x)	(EISA_BASE+(EISA_NEXT*slot))
#define MB_SLOT		0

#define BAIL(S)	{ interp->result=S; return TCL_ERROR; }
int PnPConfigCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[]);

static void PnPvendorID(FILE *out, const char *tab, const u_char *id);

u_short ioaddr[3], irq[2], dma[2];

# define DEV_OFS	0x40

int pnpFd = -1;

int PnPConfigCmd (ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{
	int deviceNumber = -1;
	int endFound = 0;
	PnP_busdata_t	pnpBus;
	PnP_findUnit_t pnpUnit;
	PnP_TagReq_t	tagReq;
	PnP_RegReq_t	regReq, targetReq;
	PnP_Active_t	pnpActive;

	u_char			*tag, regBuf[0x100U-DEV_OFS], targetRegBuf[0x100U-DEV_OFS];
	u_short			value;
	u_long			resNum, maxTagLen;
	
	int				n, slot;

	if(argc != 9)
		BAIL("expected 8 arg");

	ioaddr[0] = atoi(argv[2]); 
	ioaddr[1] = atoi(argv[3]); 
	ioaddr[2] = atoi(argv[4]); 
	irq[0] = atoi(argv[5]);
	irq[1] = atoi(argv[6]);
	dma[0] = atoi(argv[7]);
	dma[1] = atoi(argv[8]);
	targetReq.regBuf = targetRegBuf;

/*	for (n=0; n<3; n++) {
		printf("ioaddr[%d]: %d 0x%x\n", n, ioaddr[n], ioaddr[n]);
	}
*/
	if ((pnpFd = open("/dev/pnp", O_RDWR)) == -1)
	{
		printf("Cannot open %s: %s", "/dev/pnp", strerror(errno));
		return;
	}

	if (ioctl(pnpFd, PNP_BUS_PRESENT, &pnpBus) == -1)
	{
		printf("PNP_BUS_PRESENT fail: %s", strerror(errno));
		return;
	}
/*	printf("Type %i, Size: %i,  Num: %i\n", pnpBus.BusType, pnpBus.NodeSize, pnpBus.NumNodes);*/

	pnpUnit.unit.vendor = atoi(argv[1]);
	pnpUnit.unit.serial = PNP_ANY_SERIAL;
	pnpUnit.unit.unit = PNP_ANY_UNIT;
	pnpUnit.unit.node = PNP_ANY_NODE;
	if (ioctl(pnpFd, PNP_FIND_UNIT, &pnpUnit) == -1) {
		printf("PNP_FIND_UNIT fail: %s\n", strerror(errno));
		return;
	}
/*
 *	printf("vendor %d, serial %d, unit %d, node %d\n", 
 *		pnpUnit.unit.vendor,pnpUnit.unit.serial, pnpUnit.unit.unit, pnpUnit.unit.node);
*/
 	PnPvendorID(stdout, "\t", (u_char *)&pnpUnit.unit.vendor);
/*
 *	printf("NodeSize %d, ResCnt %d, DevCnt %d\n", pnpUnit.NodeSize, pnpUnit.ResCnt, pnpUnit.devCnt);
*/	
	if (pnpBus.NodeSize < 5)
		maxTagLen = USHRT_MAX + 3;
	else
		maxTagLen = pnpBus.NodeSize + 5;
	if (!(tag = malloc(maxTagLen))) {
		printf("Out of memory\n");
		return;
	}
	for (resNum=0; !endFound; resNum++) {
		memset(tag, 0, maxTagLen);
		tagReq.unit=pnpUnit.unit;

		tagReq.tagLen = maxTagLen;
		tagReq.tagPtr = tag;
		tagReq.resNum = resNum;

		if (ioctl(pnpFd, PNP_READ_TAG, &tagReq) == -1) {
			printf("PNP_READ_TAG fail: %s\n", strerror(errno));
			break;
		}
		/*printf("tagReq.tagPtr[0]=0x%x\n", tagReq.tagPtr[0]);*/

		if (!(tagReq.tagPtr[0] & 0x80U)) {
			u_short		value;
  			u_long		addr;
  			u_long		limit;
  			u_char		control;

			switch ((tagReq.tagPtr[0] >> 3) & 0x0fU) {
			case 0x02:	  /* Logical device ID */
				++deviceNumber;
				targetReq.unit = regReq.unit = tagReq.unit;
				targetReq.devNum = regReq.devNum = deviceNumber;
  				targetReq.regNum = regReq.regNum = DEV_OFS;
  				targetReq.regCnt = regReq.regCnt = sizeof(regBuf);
  								   regReq.regBuf = regBuf;
  				if (ioctl(pnpFd, PNP_READ_REG, &regReq) == -1) {
  					printf("PNP_READ_REG fail: %s\n", strerror(errno));
					break;
				}

  					for (n = 0; n < 8; ++n)		/* I/O */
  					{
						targetRegBuf[0x60-DEV_OFS+(n*2)] = ioaddr[n] >> 8;
						targetRegBuf[0x61-DEV_OFS+(n*2)] = ioaddr[n] & 0377;
  					}

  					for (n = 0; n < 2; ++n)		/* IRQ */
  					{
 						u_char	irqType = regBuf[0x71-DEV_OFS+(n*2)];

  						/* printf( "IRQ[%1d]:	   %d	  Active %s, %s trigger\n", 
						*	n, value, 
						*	irqType & 0x02U ? "high" : "low", 
						*	irqType & 0x01U ? "level" : "edge");
						*/
						targetRegBuf[0x70-DEV_OFS+(n*2)]=irq[n];
  					}
	
  					for (n = 0; n < 2; ++n)		/* DMA */
  					{
						targetRegBuf[0x74-DEV_OFS+n] = dma[n];
					}

				case 0x0f:	/* End tag */
					endFound = 1;
					break;
			}
		}
	}
	free(tag);

	if (ioctl(pnpFd, PNP_WRITE_REG, &targetReq) == -1)
		printf("PNP_WRITE_REG fail: %s\n", strerror(errno));

	pnpActive.unit = targetReq.unit;
	pnpActive.device = targetReq.devNum;
	pnpActive.active = 1;
	if (ioctl(pnpFd, PNP_WRITE_ACTIVE, &pnpActive)==-1)
		printf("PNP_WRITE_ACTIVE fail: %s\n", strerror(errno));

	close(pnpFd);
}

void
PnPvendorID(FILE *out, const char *tab, const u_char *id)
{
	u_short	prod_id;
	const char	*key;
	const char	*vendor;
	const char	*product;

	key = eisa_vendor_key(id);
	vendor = eisa_vendor_name(key);

	prod_id = ((u_short)id[2] << 8) | id[3];
	if ((product = eisa_product_name(key, prod_id)) != NULL)
		/* fprintf(out, "%s\n", product) */ ;
	else
   {
		prod_id = ((u_short)id[2] << 4) |
			((u_short)(id[3] & 0xf0) >> 4);
		/* fprintf(out, "0x%03x\n", prod_id);*/
   }
}

