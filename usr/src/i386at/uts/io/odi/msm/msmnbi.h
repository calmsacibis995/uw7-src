#ifndef _IO_MSM_MSMNBI_H   /* wrapper symbol for kernel use */
#define _IO_MSM_MSMNBI_H   /* subject to change without notice */

#ident	"@(#)msmnbi.h	2.1"
#ident  "$Header$"

typedef enum _ODI_NBI_ {
	ODI_NBI_SUCCESSFUL =		0x00000000,
	ODI_NBI_PROTECTION_VIOLATION =  0x00000001,
	ODI_NBI_HARDWARE_ERROR =	0x00000002,
	ODI_NBI_MEMORY_ERROR =	  	0x00000003,
	ODI_NBI_PARAMETER_ERROR =       0x00000004,
	ODI_NBI_UNSUPPORTED_OPERATION = 0x00000005,
	ODI_NBI_ITEM_NOT_PRESENT =      0x00000006,
	ODI_NBI_NO_MORE_ITEMS =	 	0x00000007,
	ODI_NBI_FAIL =		  	0x00000008
} ODI_NBI;

struct msmbus {
	struct msmbus			*next;
	ULONG				busTag;
	ULONG		    		busType;
	UINT32		  		physicalMemAddrSize;
	UINT32		  		ioAddrSize;
	char		    		*busName;
};

struct msmnbiinst {
	struct msmnbiinst		*next;
	UINT32				uniqueIdentifier;
	UINT16				instanceNumber;
	ULONG				busTag;
};

#define ODI_BUSTYPE_ISA		 	0
#define ODI_BUSTYPE_MCA		 	1
#define ODI_BUSTYPE_EISA		2
#define ODI_BUSTYPE_PCMCIA	      	3
#define ODI_BUSTYPE_PCI		 	4
#define ODI_BUSTYPE_VESA		5
#define ODI_BUSTYPE_NUBUS	       	6

#define BUS_SIGNATURE		   	0xAABBCCDD
#define MAX_SCAN_SEQ		    	8

#define ODI_ISA_MEMWIDTH		16
#define ODI_ISA_IOWIDTH			16
#define ODI_EISA_MEMWIDTH		32
#define ODI_EISA_IOWIDTH		32
#define ODI_PCI_MEMWIDTH		64
#define ODI_PCI_IOWIDTH			32
#define ODI_PCMCIA_MEMWIDTH		32
#define ODI_PCMCIA_IOWIDTH		32
#define ODI_MCA_MEMWIDTH		32
#define ODI_MCA_IOWIDTH			32

#define ODI_MCA_PRODUCTIDLEN		2
#define ODI_EISA_PRODUCTIDLEN		4
#define ODI_PCI_PRODUCTIDLEN		4

/*
 * flags for msm_read_eisa_config.
 */
#define MSM_EISA_SUCCESS		0x0
#define MSM_EISA_INVALID_SLOT		0x80
#define MSM_EISA_INVALID_FUNC		0x81
#define MSM_EISA_EMPTY_SLOT		0x83
#define MSM_EISA_NOMEM			0x87

#define	MSM_MATCH_SLOT			0x0
#define	MSM_MATCH_UID			0x1

#define	MSM_EISA_MCA_PARAMS		0x1
#define	MSM_PCI_PARAMS			0x2

#define	MSM_NEW_INUM			0x1
#define	MSM_GIVEN_INUM			0x2

/*
 * MSM_MAX_PARAMS should be max of above values.
 */
#define	MSM_MAX_PARAMS			0x2

/*
 * NBI routines used by both assembly and C wrappers.
 */
extern UINT32	GetAlignment(UINT32 type);
extern ODI_NBI	GetBusInfo(ULONG busTag, UINT32 *physicalMemAddrSize,
			UINT32 *ioAddrSize);
extern ODI_NBI	GetBusType(ULONG busTag, UINT32 *busType);
extern ODI_NBI	GetBusName(ULONG busTag, UINT8 **busName);
extern ODI_NBI	GetBusSpecificInfo(ULONG busTag, UINT32 size,
			void *busSpecificInfo);
extern ODI_NBI	GetBusTag(const MEON_STRING *busName, ULONG *busTag);
extern ODI_NBI	GetUniqueIdentifier(ULONG busTag, UINT32 *parameters,
			UINT32 parameterCount, UINT32 *uniqueIdentifier);
extern ODI_NBI	GetInstanceNumber(ULONG busTag, UINT32 uniqueIdentifier,
			UINT16 *instanceNumber);
extern ODI_NBI	GetInstanceNumberMapping(UINT16 instanceNumber, ULONG *busTag,
			UINT32 *uniqueIdentifier);
extern ODI_NBI	GetCardConfigInfo(ULONG busTag, UINT32 uniqueIdentifier,
			UINT32 size, UINT32 parm1, UINT32 parm2,
			void *configInfo);
extern ODI_NBI	GetSlot(ULONG busTag, MEON_STRING *slotName, UINT16 *slot);
extern ODI_NBI	GetSlotName(ULONG busTag, UINT16 slot,
			MEON_STRING **slotName);
extern ODI_NBI	ScanBusInfo(UINT32 *scanSequence, ULONG *busTag,
			UINT32 *busType, MEON **busName);
extern ODI_NBI	SearchAdapter(UINT32 *scanSequence, UINT32 busType,
			UINT32 productIDlen, const MEON *productID,
			ULONG *busTag, UINT16 *slot, UINT32 *uniqueIdentifier);
UINT8		RdConfigSpace8(ULONG busTag, UINT32 uniqueIdentifier,
			UINT32 offset);
UINT16		RdConfigSpace16(ULONG busTag, UINT32 uniqueIdentifier,
			UINT32 offset);
UINT32		RdConfigSpace32(ULONG busTag, UINT32 uniqueIdentifier,
			UINT32 offset);
void		WrtConfigSpace8(ULONG busTag, UINT32 uniqueIdentifier,
			UINT32 offset, UINT8 writeVal);
void		WrtConfigSpace16(ULONG busTag, UINT32 uniqueIdentifier,
			UINT32 offset, UINT16 writeVal);
void		WrtConfigSpace32(ULONG busTag, UINT32 uniqueIdentifier,
			UINT32 offset, UINT32 writeVal);
/*
 * NBI routines exported directly out of the C spec (not used by assembly glu)
 */
void		DMACleanup(UINT32 DMAChannel);
ODI_NBI		DMAStart(void *destBusTag, UINT32 destAddrType, const void
			*destAddr, void *srcBusTag, UINT32 srcAddrType,
			const void *srcAddr, UINT32 len, UINT32 DMAChannel,
			UINT32 DMAMode1, UINT32 DMAMode2);
UINT32		DMAStatus(UINT32 DMAChannel);
ODI_NBI		FreeBusMemory(void *busTag1, const void *memAddrPtr,
			void *busTag2, const void *mappedAddrPtr,
			UINT32 len);

UINT8		In8(void *busTag, const void *IOAddrPtr);
UINT16		In16(void *busTag, const void *IOAddrPtr);
UINT32		In32(void *busTag, const void *IOAddrPtr);
ODI_NBI		InBuff8(UINT8 *bufferPtr, void *IOBusTag,
			const void *IOAddrPtr, UINT32 count);
ODI_NBI		InBuff16(UINT16 *bufferPtr, void *IOBusTag,
			const void *IOAddrPtr, UINT32 count);
ODI_NBI		InBuff32(UINT32 *bufferPtr, void *IOBusTag,
			const void *IOAddrPtr, UINT32 count);
ODI_NBI		MapBusMemory(void *busTag1, const void *memAddrPtr,
			void *busTag2, void *mappedAddrPtr, UINT32 len);
ODI_NBI		MovFastFromBus(void *destAddrPtr, void *srcBusTag,
			const void *reserved, const void *srcAddr,
			UINT32 count);
ODI_NBI		MovFastToBus(void *destBusTag, void *reserved, void *destAddr,
			const void *srcAddrPtr, UINT32 count);
ODI_NBI		MovFromBus8(void *destAddrPtr, void *srcBusTag, void
			*reserved, const void *srcAddr, UINT32 count);
ODI_NBI		MovFromBus16(void *destAddrPtr, void *srcBusTag,
			void *reserved, const void *srcAddr, UINT32 count);
ODI_NBI		MovFromBus32(void *destAddrPtr, void *srcBusTag, void
			*reserved, const void *srcAddr, UINT32 count);
ODI_NBI		MovToBus8(void *destBusTag, void *reserved, void *destAddr,
			const void *srcAddrPtr, UINT32 count);
ODI_NBI		MovToBus16(void *destBusTag, void *reserved, void *destAddr,
			const void *srcAddrPtr, UINT32 count);
ODI_NBI		MovToBus32(void *destBusTag, void *reserved, void *destAddr,
			const void *srcAddrPtr, UINT32 count);
void		Out8(void *busTag, const void *IOAddrPtr, UINT8 outputVal);
void		Out16(void *busTag, const void *IOAddrPtr, UINT16 outputVal);
void		Out32(void *busTag, const void *IOAddrPtr, UINT32 outputVal);
ODI_NBI		OutBuff8(void *IOBusTag, void *IOAddrPtr, const void
			*bufferPtr, UINT32 count);
ODI_NBI		OutBuff16(void *IOBusTag, void *IOAddrPtr, const void
			*bufferPtr, UINT32 count);
ODI_NBI		OutBuff32(void *IOBusTag, void *IOAddrPtr, const void
			*bufferPtr, UINT32 count);
UINT8		Rd8(void *busTag, const void *reserved, const void *memAddr);
UINT16		Rd16(void *busTag, const void *reserved, const void *memAddr);
UINT32		Rd32(void *busTag, const void *reserved, const void *memAddr);
ODI_NBI		Set8(void *busTag, const void *reserved, const void *memAddr,
			UINT8 value, UINT32 count);
ODI_NBI		Set16(void *busTag, const void *reserved, const void *memAddr,
			UINT16 value, UINT32 count);
ODI_NBI		Set32(void *busTag, const void *reserved, const void
			*memLogicalAddr, UINT32 value, UINT32 count);
void		Slow(void);
void		Wrt8(void *busTag, void *reserved, void *memAddr,
			UINT8 writeVal);
void		Wrt16(void *busTag, void *reserved, void *memAddr,
			UINT16 writeVal);
void		Wrt32(void *busTag, void *reserved, void *memAddr,
			UINT32 writeVal);
UINT32		UnMaskBusInterrupt(void *busTag, UINT8 interrupt);
UINT32		MaskBusInterrupt(void *busTag, UINT8 interrupt);


/*
 * asms needed to support NBI.
 */

__asm void gen_MovFastFromBus(void *destAddr, const void *srcAddr, UINT32 count)
{
%mem destAddr, srcAddr, count;
	pushl	%edi			/ save %edi
	pushl	%esi			/ save %esi
	movl	destAddr, %edi
	movl	srcAddr, %esi
	movl	count, %ecx
	movl	%ecx, %eax
	shrl	$0x02, %ecx
	rep;	smovl
	movl	%eax, %ecx
	andl	$0x03, %ecx
	rep;	smovb
	popl	%esi			/ restore %esi
	popl	%edi			/ restore %edi
}

__asm void gen_MovFastToBus(void *destAddr, const void *srcAddr, UINT32 count)
{
%mem destAddr, srcAddr, count;
	pushl	%edi			/ save %edi
	pushl	%esi			/ save %esi
	movl	destAddr, %edi
	movl	srcAddr, %esi
	movl	count, %ecx
	movl	%ecx, %eax
	shrl	$0x02, %ecx
	rep;	smovl
	movl	%eax, %ecx
	andl	$0x03, %ecx
	rep;	smovb
	popl	%esi			/ restore %esi
	popl	%edi			/ restore %edi
}

__asm void gen_Mov8(void *destAddr, const void *srcAddr, UINT32 count)
{
%mem destAddr, srcAddr, count;
	pushl	%edi			/ save %edi
	pushl	%esi			/ save %esi
	movl	destAddr, %edi
	movl	srcAddr, %esi
	movl	count, %ecx
	rep;	smovb
	popl	%esi			/ restore %esi
	popl	%edi			/ restore %edi
}

__asm void gen_Mov16(void *destAddr, const void *srcAddr, UINT32 count)
{
%mem destAddr, srcAddr, count;
	pushl	%edi			/ save %edi
	pushl	%esi			/ save %esi
	movl	destAddr, %edi
	movl	srcAddr, %esi
	movl	count, %ecx
	rep;	smovw
	popl	%esi			/ restore %esi
	popl	%edi			/ restore %edi
}

__asm void gen_Mov32(void *destAddr, const void *srcAddr, UINT32 count)
{
%mem destAddr, srcAddr, count;
	pushl	%edi			/ save %edi
	pushl	%esi			/ save %esi
	movl	destAddr, %edi
	movl	srcAddr, %esi
	movl	count, %ecx
	rep;	smovl
	popl	%esi			/ restore %esi
	popl	%edi			/ restore %edi
}

__asm UINT8 gen_Rd8(const void *memAddr)
{
%mem memAddr;
	movl	memAddr, %eax
	movb	(%eax), %al
}

__asm UINT16 gen_Rd16(const void *memAddr)
{
%mem memAddr;
	movl	memAddr, %eax
	movw	(%eax), %ax
}

__asm UINT32 gen_Rd32(const void *memAddr)
{
%mem memAddr;
	movl	memAddr, %eax
	movl	(%eax), %eax
}

__asm void gen_Set8(const void *memAddr, UINT8 value, UINT32 count)
{
%mem memAddr, value, count;
	pushl	%edi			/ save %edi
	pushl	%esi			/ save %esi
	movl	memAddr, %edi
	movb	value, %al
	movl	count, %ecx
	rep;	stosb
	popl	%esi			/ restore %esi
	popl	%edi			/ restore %edi
}

__asm void gen_Set16(const void *memAddr, UINT16 value, UINT32 count)
{
%mem memAddr, value, count;
	pushl	%edi			/ save %edi
	pushl	%esi			/ save %esi
	movl	memAddr, %edi
	movw	value, %ax
	movl	count, %ecx
	rep;	stosw
	popl	%esi			/ restore %esi
	popl	%edi			/ restore %edi
}

__asm void gen_Set32(const void *memAddr, UINT32 value, UINT32 count)
{
%mem memAddr, value, count;
	pushl	%edi			/ save %edi
	pushl	%esi			/ save %esi
	movl	memAddr, %edi
	movl	value, %eax
	movl	count, %ecx
	rep;	stosl
	popl	%esi			/ restore %esi
	popl	%edi			/ restore %edi
}

__asm void gen_Wrt8(void *memAddr, UINT8 writeVal) 
{
%mem memAddr, writeVal;
	movl	memAddr, %ecx
	movb	writeVal, %al
	movb	%al, (%ecx)
}

__asm void gen_Wrt16(void *memAddr, UINT16 writeVal) 
{
%mem memAddr, writeVal;
	movl	memAddr, %ecx
	movw	writeVal, %ax
	movw	%ax, (%ecx)
}

__asm void gen_Wrt32(void *memAddr, UINT32 writeVal) 
{
%mem memAddr, writeVal;
	movl	memAddr, %ecx
	movl	writeVal, %eax
	movl	%eax, (%ecx)
}

#endif	/* _IO_MSM_MSMNBI_H */
