
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <sys/pnp.h>
#include <limits.h>

#include "hw_eisa.h"
#include "eisa_data.h"

#define EISA_MB_ID	0xfffd9		/* f000:ffd9 "EISA" */
#define NO_EISA_ID	0xffffffff

#define	EISA_BASE	0x0c80		/* base address of EISA IDs */
#define	EISA_SLOTS	16		/* slots including MB */
#define EISA_NEXT	0x1000		/* Offset to next adapter */
#define	EISA_SLOT(x)	(EISA_BASE+(EISA_NEXT*slot))
#define MB_SLOT		0

static int		functionNumber = -1;
static int		deviceNumber = -1;
static void PnPvendorID(FILE *out, const char *tab, const u_char *id);

const u_short ioaddr[3] = {0x220, 0x330, 0x388};
const u_short irq[1] = {5};
const u_short dma[2] = {1, 5};

/* const char	* const callme_eisa[] =
{
	"eisa",
	"eisa_bus",
	"eisa_buss",
	NULL
};

const char	short_help_eisa[] = "Info on EISA bus devices";*/

static u_long		eisa_mb_id = 0;
static const size_t	MbIdLen = sizeof(eisa_mb_id);

/*
int
have_eisa(void)
{
	static const u_long		mb_sig = 0x41534945;	/* "EISA" *

	if (!eisa_mb_id)
	{
	/*
	 * eisa_mb_id = *(u_long *)ptok(EISA_MB_ID);
	 *

	if (read_mem(EISA_MB_ID, &eisa_mb_id, MbIdLen) != MbIdLen)
	{
		debug_print("EISA signature read fail");
		eisa_mb_id = NO_EISA_ID;
	}
	else
		debug_print("EISA signature: 0x%8.8lx", eisa_mb_id);
	}

	return (eisa_mb_id == mb_sig) ? 1 : 0;
}*/

static const char *
devTab()
{
	if (deviceNumber >= 0)
	return (functionNumber >= 0) ? "\t\t" : "\t	";

	return "\t";
}

# define DEV_OFS	0x40

int pnpFd = -1;
int endFound=0;

void main ()
{
	PnP_busdata_t	pnpBus;
	PnP_findUnit_t pnpUnit;
	PnP_TagReq_t	tagReq;
	PnP_RegReq_t	regReq, targetReq;
	PnP_Active_t	pnpActive;

	u_char			*tag, regBuf[0x100U-DEV_OFS], targetRegBuf[0x100U-DEV_OFS];
	u_short			value;
	u_long			resNum, maxTagLen;
	
	int				n, slot;

	targetReq.regBuf = targetRegBuf;

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
	printf("Type %i, Size: %i,  Num: %i\n", pnpBus.BusType, pnpBus.NodeSize, pnpBus.NumNodes);

	pnpUnit.unit.vendor = PNP_ANY_VENDOR;
	pnpUnit.unit.serial = PNP_ANY_SERIAL;
	pnpUnit.unit.unit = PNP_ANY_UNIT;
	pnpUnit.unit.node=1;
	if (ioctl(pnpFd, PNP_FIND_UNIT, &pnpUnit) == -1) {
		printf("PNP_FIND_UNIT fail: %s\n", strerror(errno));
		return;
	}

	printf("vendor %d, serial %d, unit %d, node %d\n", 
		pnpUnit.unit.vendor,pnpUnit.unit.serial, pnpUnit.unit.unit, pnpUnit.unit.node);
	PnPvendorID(stdout, "\t", (u_char *)&pnpUnit.unit.vendor);
	printf("NodeSize %d, ResCnt %d, DevCnt %d\n", pnpUnit.NodeSize, pnpUnit.ResCnt, pnpUnit.devCnt);
	
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

		if (!(tagReq.tagPtr[0] & 0x80U)) {
			u_short		value;
  			u_long		addr;
  			u_long		limit;
  			u_char		control;
  			int			found = 0;

			switch ((tagReq.tagPtr[0] >> 3) & 0x0fU) {
				printf("0x%x\n", tagReq.tagPtr[0]);
			case 0x02:	  /* Logical device ID */
  				printf("\tDevice number:		%lu\n", ++deviceNumber);
				targetReq.unit = regReq.unit = tagReq.unit;
				targetReq.devNum = regReq.devNum = deviceNumber;
  				targetReq.regNum = regReq.regNum = DEV_OFS;
  				targetReq.regCnt = regReq.regCnt = sizeof(regBuf);
  								   regReq.regBuf = regBuf;
  				if (ioctl(pnpFd, PNP_READ_REG, &regReq) == -1) {
  					printf("PNP_READ_REG fail: %s\n", strerror(errno));
					break;
				}

  				printf("\n%sCurrent settings\n", devTab());
  				/*debug_dump(regBuf, sizeof(regBuf), DEV_OFS, "Registers");*/

  				for (n = 0; n < 4; ++n)		/* 24 bit Memory */
  				{
					addr = ((u_long)regBuf[0x40-DEV_OFS+(n*0x08U)] << 16) |
  	   				((u_long)regBuf[0x41-DEV_OFS+(n*0x08U)] << 8);
				
					if (addr)
					{
						control = regBuf[0x42-DEV_OFS+(n*0x08U)];

	  					limit = ((u_long)regBuf[0x43-DEV_OFS+(n*0x08U)] << 16) |
  			  				((u_long)regBuf[0x44-DEV_OFS+(n*0x08U)] << 8);

 		  				if (control & 0x01)
						limit = addr + limit - 1;	/* Range -> Limit */

  		  				printf("%s	MEM[%1d]:	   0x%8.8lx - 0x%8.8lx   %d bit\n",
						devTab(), n, addr, limit, control & 0x02U ? 16 : 8);
  		  				found = 1;
					}
  					for (n = 0; n < 8; ++n)		/* I/O */
  					{
						value = ((u_short)regBuf[0x60-DEV_OFS+(n*2)] << 8) | (u_short)regBuf[0x61-DEV_OFS+(n*2)];

						if (value)
						{
  		  					printf("%s	I/O[%1d]:	   0x%4.4hx\n", devTab(), n, value);
  		 					found = 1;
							targetRegBuf[0x60-DEV_OFS+(n*2)] = ioaddr[n] >> 8;
							targetRegBuf[0x61-DEV_OFS+(n*2)] = ioaddr[n] & 0377;
						}
  					}

  					for (n = 0; n < 2; ++n)		/* IRQ */
  					{
						value = regBuf[0x70-DEV_OFS+(n*2)];
	
						if (value)
						{
  							u_char	irqType = regBuf[0x71-DEV_OFS+(n*2)];
	
  							printf( "%s	IRQ[%1d]:	   %d	  Active %s, %s trigger\n", 
								devTab(), n, value, 
								irqType & 0x02U ? "high" : "low", 
								irqType & 0x01U ? "level" : "edge");
							targetRegBuf[0x70-DEV_OFS+(n*2)]=irq[n];
  							found = 1;
						}
  					}
	
  					for (n = 0; n < 2; ++n)		/* DMA */
  					{
						value = regBuf[0x74-DEV_OFS+n] & 0x07U;
	
						if (value != 4)
						{
							printf("%s	DMA[%1d]:	   %d\n", devTab(), n, value);
							found = 1;
							targetRegBuf[0x74-DEV_OFS+n] = dma[n];
						}
					}

				case 0x0f:	/* End tag */
					printf("%sChecksum:		 0x%2.2x\n", devTab(), tag[1]);
					endFound = 1;
					break;
				}
			}
			if (!found) printf("%s	No resources assigned\n", devTab());
		}
		free(tag);
	}

	if (ioctl(pnpFd, PNP_WRITE_REG, &targetReq) == -1)
		printf("PNP_WRITE_REG fail: %s\n", strerror(errno));

	pnpActive.unit = targetReq.unit;
	pnpActive.device = targetReq.devNum;
	printf("%i\n", pnpActive.device);
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
	fprintf(out, "%sVendor:		   %s\n",
		tab,
		(vendor = eisa_vendor_name(key)) ? vendor : key);

	fprintf(out, "%sProduct:		  ", tab);
	prod_id = ((u_short)id[2] << 8) | id[3];
	if ((product = eisa_product_name(key, prod_id)) != NULL)
		fprintf(out, "%s\n", product);
	else
   {
		prod_id = ((u_short)id[2] << 4) |
			((u_short)(id[3] & 0xf0) >> 4);
		fprintf(out, "0x%03x\n", prod_id);
   }
}

/*void
debug_dump(const void *buf, u_long len, u_long rel_adr, const char *fmt, ...)
{
    register int	x, y;
    u_char		text[DMPLEN];
    va_list		args;
    int			state;
    const u_char	*adr = (u_char *)buf;


    if (!debug)
	return;

    va_start(args, fmt);
    vfprintf(errorFd, fmt, args);
    va_end(args);
    fprintf(errorFd, " length: %lu", len);

    if (!len)
    {
	fprintf(errorFd, "\n");
	return;
    }
*/
