#ifndef _IO_MSM_MSMPORTABLE_H	/* wrapper symbol for kernel use */
#define _IO_MSM_MSMPORTABLE_H	/* subject to change without notice */

#ident	"@(#)msmportable.h	2.1"
#ident	"$Header$"

/*
 *	portable.h, provides definitions that support C code portablity
 *	in various environments. currently used for ODI, but should
 *	probably be merged with other such definitions.
 */

/*
 * this section defines the portability characteristics of each
 * machine type. Note: you should define one of these machine
 * types prior to including portable.h.
 */

#ifdef  IAPX386		/* X86 */
#define        LO_HI_MACH_TYPE
#endif

#ifdef  MC680X0		/* MOT 68K */
#define        HI_LO_MACH_TYPE
#endif

#ifdef  MC88000        /* IBM RISC */
#define        HI_LO_MACH_TYPE
#define        STRICT_ALIGNMENT
#endif

#ifdef  RX000		/* MIPS */
#define        HI_LO_MACH_TYPE
#define        STRICT_ALIGNMENT
#endif

#ifdef  SPARC		/* SUN */
#define        HI_LO_MACH_TYPE
#define        STRICT_ALIGNMENT
#endif

#ifdef	HPPA		/* HP PA-RISC */
#define        HI_LO_MACH_TYPE
#define        STRICT_ALIGNMENT
#endif

/*
 * netware integer types.
 */
#ifndef LONG
#define  LONG unsigned long
#endif

#ifndef WORD
#define  WORD unsigned short
#endif

#ifndef BYTE
#define  BYTE unsigned char
#endif

/* 
 * This section of portable.h will attempt to create a consistant
 * set of macros to isolate the alignment and endianness of a machine 
 * from the C code. Each macro will follow a consistant naming 
 * convention and then allow aliases to support previous versions 
 * of byte swapping macros and routines.  
 *
 * The following macros use WORD to mean 2 bytes and LONG to mean
 * 4 bytes. Where macros operate on more than 4 bytes the number 
 * of bytes is included in the name (ex. COPY_6BYTE).
 *
 * The word 'addr' is used to indicate a byte pointer to the data
 * that will be acted upon. A variable is indicated by 'var', and
 * 'value' is used where either a constant or a variable can be used.
 */


/*
 * The first set of macros is used to copy un-aligned data from
 * one address to another. They do not swap the data and can be
 * called on either aligned or un-aligned data.
 *
 * COPY_2BYTE(src_addr, dest_addr)	
 * COPY_WORD(src_addr, dest_addr)	
 * COPY_SOCKET(src_addr, dest_addr)	
 *
 * COPY_4BYTE(src_addr, dest_addr)
 * COPY_LONG(src_addr, dest_addr)
 * COPY_NET(src_addr, dest_addr)
 *
 * COPY_6BYTE(src_addr, dest_addr)
 * COPY_NODE(src_addr, dest_addr)
 * COPY_HOST(src_addr, dest_addr)
 *
 * COPY_12BYTE(src_addr, dest_addr)
 * COPY_ADDRESS(src_addr, dest_addr)
 *
 *
 * The next set of macros is used to assign a variable a value 
 * that may be mis-aligned. They do not swap the data.
 *
 * addr is the address of the potentially mis-aligned data.
 * value can be either a constant or a variable.
 *
 * var = GET_WORD(addr)	
 * var = GET_LONG(addr)
 * PUT_WORD(value, addr)
 * PUT_LONG(value, addr)
 *
 * Next is a set of macros that perform alignment and byte swaps if needed.  
 * The macros containing HILO will swap bytes on Little Endian machines only.  
 * The macros containing LOHI will swap bytes on Big Endian machines only.  
 *
 * var = GET_HILO_WORD(addr)
 * 		var = GET_LEN(addr)		alias
 * 		var = GET_SOCKET(addr)		alias
 * var = GET_HILO_LONG(addr)
 * var = GET_LOHI_WORD(addr)
 * var = GET_LOHI_LONG(addr)
 * PUT_HILO_WORD(value, addr)
 * 		PUT_LEN(value, addr)		alias
 * 		PUT_SOCKET(value, addr)		alias
 * PUT_HILO_LONG(value, addr)
 * PUT_LOHI_WORD(value, addr)
 * PUT_LOHI_LONG(value, addr)
 *
 *
 * Given a value (ie constants, register variables, or expressions)
 * the next set of macros will return the value in specified order.
 * The macros containing HILO will return the value in HILO order.  
 * The macros containing LOHI will return the value in LOHI order.  
 *
 * var = VALUE_TO_HILO_WORD(value)
 *		WORD_TO_HILO(word)		alias
 *		SOCKET_NUMBER(hostorder_sock)	alias
 * var = VALUE_TO_HILO_LONG(value)
 * var = VALUE_TO_LOHI_WORD(value)
 * var = VALUE_TO_LOHI_LONG(value)
 * var = VALUE_FROM_HILO_WORD(value)
 * var = VALUE_FROM_HILO_LONG(value)
 * var = VALUE_FROM_LOHI_WORD(value)
 * var = VALUE_FROM_LOHI_LONG(value)
 *
 *
 * The next set of macros will be used less frequently but are included for
 * cases where the macros above will not work. They copy data from a source
 * address to a destination address swapping and/or aligning data as needed.
 * The macros containing HILO will swap bytes on Little Endian machines only.  
 * The macros containing LOHI will swap bytes on Big Endian machines only.  
 *
 * COPY_TO_HILO_WORD(src_addr, dest_addr)
 * COPY_TO_HILO_LONG(src_addr, dest_addr)
 * COPY_TO_LOHI_WORD(src_addr, dest_addr)
 * COPY_TO_LOHI_LONG(src_addr, dest_addr)
 * COPY_FROM_HILO_WORD(src_addr, dest_addr)
 * COPY_FROM_HILO_LONG(src_addr, dest_addr)
 * COPY_FROM_LOHI_WORD(src_addr, dest_addr)
 * COPY_FROM_LOHI_LONG(src_addr, dest_addr)
 * 
 * Alternately, when the source and destination address are the same the
 * following macros can be used.  
 *
 * HOST_TO_HILO_WORD(addr)
 * HOST_TO_HILO_LONG(addr)
 * HOST_TO_LOHI_WORD(addr)
 * HOST_TO_LOHI_LONG(addr)
 * HOST_FROM_HILO_WORD(addr)
 * HOST_FROM_HILO_LONG(addr)
 * HOST_FROM_LOHI_WORD(addr)
 * HOST_FROM_LOHI_LONG(addr)
 *
 *
 * Finally a set of macros to compare two groups of bytes to see if they
 * are equal are included. They return TRUE if equal.
 * 
 * ARE_2BYTES_EQUAL(addr1, addr2)
 *		WORD_EQUAL(addr1, addr2)		alias
 * ARE_4BYTES_EQUAL(addr1, addr2)
 *		LONG_EQUAL(addr1,addr2)			alias
 *		NET_EQUAL(addr1,addr2)			alias
 * ARE_6BYTES_EQUAL(addr1, addr2)
 *		NODE_EQUAL(addr1,addr2)			alias
 *		HOST_EQUAL(addr1,addr2)			alias
 * ARE_12BYTES_EQUAL(addr1, addr2)
 *		ADDRESS_EQUAL(addr1,addr2)		alias
 *
 *
 */

/*
 *	COPY_2BYTE(src_addr, dest_addr)
 *	COPY_WORD(src_addr, dest_addr)				alias
 *	COPY_SOCKET(src_addr, dest_addr)			alias
 *
 *	COPY_4BYTE(src_addr, dest_addr)
 *	COPY_LONG(src_addr, dest_addr)				alias
 *	COPY_NET(src_addr, dest_addr)				alias
 *
 *	COPY_6BYTE(src_addr, dest_addr)
 *	COPY_NODE(src_addr, dest_addr)				alias
 *	COPY_HOST(src_addr, dest_addr)				alias
 *
 *	COPY_12BYTE(src_addr, dest_addr)
 *	COPY_ADDRESS(src_addr, dest_addr)			alias
 *
 *	Description:
 *		Copy 2, 4, 6 or 12 bytes of potentially misaligned data.
 *
 *	Purpose:
 *		Useful for copying sockets, networknumbers, nodenumbers
 *		or complete ipx addresses between ipx packets or between
 *		ipx packets and some location in memory that is in hilo order.
 *
 *	Example:
 *		..
 *		COPY_NET(&reqpacket->srcnet, &resppacket->dstnet);
 *		COPY_NODE(&reqpacket->srcnode, &resppacket->dstnode);
 *		COPY_SOCKET(&reqpacket->srcsocket, &resppacket->dstsocket);
 *		..
 *		COPY_ADDRESS(&reqpacket->srcnet, &resppacket->dstnet);
 *		..
 */		

/*
 * first define some structures that make copying misaligned data easy.
 */
typedef struct {
	BYTE bytes[2];
} _C2T_;

typedef struct {
	BYTE bytes[4];
} _C4T_;

typedef struct {
	BYTE bytes[6];
} _C6T_;

typedef struct {
	BYTE bytes[12];
} _C12T_;

/*
 * now use the structures to do inline copies.
 */
#define COPY_2BYTE(src, dest) (*((_C2T_ *)(dest)) = *((_C2T_ *)(src)))
#define COPY_4BYTE(src, dest) (*((_C4T_ *)(dest)) = *((_C4T_ *)(src)))
#define COPY_6BYTE(src, dest) (*((_C6T_ *)(dest)) = *((_C6T_ *)(src)))
#define COPY_12BYTE(src, dest) (*((_C12T_ *)(dest)) = *((_C12T_ *)(src)))

/*
 * aliases.
 */
#define COPY_WORD(src, dest)	COPY_2BYTE(src, dest)
#define COPY_SOCKET(src, dest) 	COPY_2BYTE(src, dest)

#define COPY_LONG(src, dest) 	COPY_4BYTE(src, dest)
#define COPY_NET(src, dest) 	COPY_4BYTE(src, dest)

#define COPY_NODE(src, dest) 	COPY_6BYTE(src, dest)
#define COPY_HOST(src, dest) 	COPY_6BYTE(src, dest)

#define COPY_ADDRESS(src_addr, dest_addr) COPY_12BYTE(src_addr, dest_addr)

/*
 *	GET_WORD(address)
 *
 *	Description:
 *		Given an address of a 2 byte word in memory,
 *		get it without changing its byte order.
 *		Address may be misaligned.
 *
 *	Purpose:
 *		Useful for picking a socket number out of a packet
 *		header without changing its byte order.
 *
 *	Example:
 *		WORD socketinhiloorder;
 *
 *		socketinhiloorder = GET_WORD(&ipxpacket->dstsocket);
 *
 *	Note:
 *		Beware of side effects in address since it may be evaluated
 *		twice.
 */

/*
 * if this machine is not strictly aligned then do a simple fetch.
 * otherwise, if the machine's byte order is hilo then do GET_HILO_WORD.
 * otherwise, get it a byte at a time.
 */ 
#if !defined(STRICT_ALIGNMENT)
	#define GET_WORD(addr) (*(WORD *)(addr))
#else
	#if defined(HI_LO_MACH_TYPE)
		#define GET_WORD(addr) GET_HILO_WORD(addr)
	#else
		#define GET_WORD(addr) (				\
		((BYTE *)(addr))[0] |					\
		((BYTE *)(addr))[1]	<< 8				\
	) 
	#endif
#endif

/*
 *	GET_LONG(address)
 *
 *	Description:
 *		Given an address of a 4 byte long in memory,
 *		get it without changing its byte order.
 *		Address may be misaligned.
 *
 *	Purpose:
 *		Useful for picking a network number out of a packet
 *		header without changing its byte order.
 *		This macro is used more than GET_HILO_LONG on
 *		network numbers because ipx/rip/sap generally leave
 *		network numbers in hilo order.
 *
 *	Example:
 *		LONG networkinhiloorder;
 *
 *		networkinhiloorder = GET_LONG(&ipxpacket->dstnetwork);
 *
 *	Note:
 *		Beware of side effects in address since it may be evaluated
 *		four times.
 */

/*
 * if this machine is not strictly aligned then do a simple fetch.
 * otherwise, if the machine's byte order is hilo then do GET_HILO_LONG.
 * otherwise, get it a byte at a time.
 */ 
#if !defined(STRICT_ALIGNMENT)
	#define GET_LONG(addr) (*(LONG *)(addr))
#else
	#if defined(HI_LO_MACH_TYPE)
		#define GET_LONG(addr) GET_HILO_LONG(addr)
	#else
		#define GET_LONG(addr) (				\
		((BYTE *)(addr))[0] 	  |				\
		((BYTE *)(addr))[1] << 8  |				\
		((BYTE *)(addr))[2] << 16 |				\
		((BYTE *)(addr))[3] << 24 				\
	) 
	#endif
#endif

/*
 *	PUT_WORD(word, address)
 *
 *	Description:
 *		Given a 2 byte word and an address in memory,
 *		store the word in memory without changing its byte order.
 *		Address may be misaligned.
 *
 *	Purpose:
 *		Useful for stuffing constants or variables that are already
 *		in hilo order in an ipxpacket in hilo order.
 *		Similar to COPY_WORD but works on constants and expressions
 *		COPY_WORD requires the src to be an address.
 *
 *	Example:
 *		PUT_WORD(CONSTANT_ALREADY_IN_HILO_ORDER, &routeans->Operation)
 *
 *	Note:
 *		Beware of side effects in both word and address since
 *		they may each be evaluated twice.
 */

/*
 * if this machine is not strictly aligned then do a simple store.
 * otherwise, if the machine's byte order is hilo then do PUT_HILO_WORD.
 * otherwise, store it a byte at a time.
 */ 
#if !defined(STRICT_ALIGNMENT)
	#define PUT_WORD(w, addr) (					\
	*((WORD *)(addr)) = (WORD)(w)					\
	)
#else
	#if defined(HI_LO_MACH_TYPE)
		#define PUT_WORD(w, addr) PUT_HILO_WORD(w, addr)
	#else
		#define PUT_WORD(w, addr) (				\
		((BYTE *)(addr))[0] = (BYTE)(w),			\
		((BYTE *)(addr))[1] = (BYTE)((w) >> 8)			\
	) 
	#endif
#endif

/*
 *	PUT_LONG(long, address)
 *
 *	Description:
 *		Given a 4 byte long and an address in memory,
 *		store the long in memory without changing its byte order.
 *		Address may be misaligned.
 *
 *	Purpose:
 *		Useful for stuffing constants or variables that are already
 *		in hilo order in an ipxpacket in hilo order.
 *		Similar to COPY_LONG but works on constants and expressions.
 *		COPY_LONG requires the src to be an address.
 *
 *	Example:
 *		PUT_LONG(network_in_hilo_order, &ipxpacket->DstNet)
 *
 *	Note:
 *		Beware of side effects in both long and address since
 *		they may each be evaluated four times.
 */

/*
 * if this machine is not strictly aligned then do a simple store.
 * otherwise, if the machine's byte order is hilo then do PUT_HILO_LONG.
 * otherwise, store it a byte at a time.
 */ 
#if !defined(STRICT_ALIGNMENT)
	#define PUT_LONG(l, addr) (					\
	*((LONG *)(addr)) = (LONG)(l)					\
	)
#else
	#if defined(HI_LO_MACH_TYPE)
		#define PUT_LONG(l, addr) PUT_HILO_LONG(l, addr)
	#else
		#define PUT_LONG(l, addr) (				\
		((BYTE *)(addr))[0] = (BYTE)(l),			\
		((BYTE *)(addr))[1] = (BYTE)((l) >> 8),			\
		((BYTE *)(addr))[2] = (BYTE)((l) >> 16),		\
		((BYTE *)(addr))[3] = (BYTE)((l) >> 24)			\
	) 
	#endif
#endif

/**************************************************************************/

/*
 *	GET_HILO_WORD(address)
 *
 *	Description:
 *		Given an address of a 2 byte word in hilo order in memory,
 *		return its value in host order.
 *		Address may be misaligned.
 *
 *	Purpose:
 *		Useful for dealing with length and socket number fields in
 *		ipx headers and fuction fields in sap and rip packets.
 *
 *	Example:
 *		WORD len;
 *		..
 *		len = GET_HILO_WORD(&ipxpacketptr->length);
 *		if (len > MAX_PACKET_SIZE)
 *			goto BadPacket;
 *		..
 *
 *	Note:
 *		Beware of side effects of addr since it will
 * 		be evaluated twice.
 */

/*
 * if this machine's byte order is hilo and it is not strictly aligned
 * then do a simple fetch. otherwise do it a byte at a time.
 */
#if defined(HI_LO_MACH_TYPE) && !defined(STRICT_ALIGNMENT)
	#define GET_HILO_WORD(addr) (*(WORD *)(addr))
#else
	#define GET_HILO_WORD(addr) (					\
	((BYTE *)(addr))[0] << 8 |					\
	((BYTE *)(addr))[1]						\
	) 
#endif

/*
 * aliases.
 */
#define GET_LEN(ptr_to_len) GET_HILO_WORD(ptr_to_len)
#define GET_SOCKET(ptr_to_socket) GET_HILO_WORD(ptr_to_socket)

/*
 *	GET_HILO_LONG(address)
 *
 *	Description:
 *		Given an address of a 4 byte long in hilo order in memory,
 *		return its value in host order.
 *		Address may be misaligned.
 *
 *	Purpose:
 *		Useful for dealing with network numbers in ipx packets
 *		when host order is required.  Typically this is only
 *		when doing compares or other arithmetic operations.  This
 *		is the case because ipx/router/sap usually keep network 
 *		numbers in hilo order so a conversion to host order is
 *		not usually neccessary.
 *
 *	Example:
 *		LONG netnumber;
 *		..
 *		netnumber = GET_HILO_LONG(&ipxpacketptr->srcnetnumber);
 *		RoutineThatNeedsNetnumberInHostOrder(ipxpacket, netnumber);
 *		..
 *
 *	Note:
 *		Beware of side effects of address since it will
 * 		be evaluated four times.
 */

/*
 * if this machine's byte order is hilo and it is not strictly aligned
 * then do a simple fetch. otherwise do it a byte at a time.
 */
#if defined(HI_LO_MACH_TYPE) && !defined(STRICT_ALIGNMENT)
	#define GET_HILO_LONG(addr) (*(LONG *)(addr))
#else
	#define GET_HILO_LONG(addr) (					\
	((BYTE *)(addr))[0] << 24 |					\
	((BYTE *)(addr))[1] << 16 |					\
	((BYTE *)(addr))[2] << 8  |					\
	((BYTE *)(addr))[3]						\
	) 
#endif

/*
 *	PUT_HILO_WORD(word, address)
 *	PUT_LEN(word, ptr_to_len)			Alias
 *	PUT_SOCKET(word, ptr_to_socket)		Alias
 *
 *	Description:
 *		Given a 2 byte word in host order and an address in memory,
 *		store the word in memory in hilo order.
 *		Address may be misaligned.
 *
 *	Purpose:
 *		Useful for stuffing the length in an ipx packet and
 *		function in rip and sap packets.
 *
 *	Example:
 *		PUT_HILO_WORD(2, &routeans->Operation)
 *
 *	Note:
 *		Beware of side effects in both word and address since
 *		they may each be evaluated twice.
 */

/*
 * if this machine's byte order is hilo and it is not strictly aligned
 * then do a simple store. otherwise do it a byte at a time.
 */
#if defined(HI_LO_MACH_TYPE) && !defined(STRICT_ALIGNMENT)
	#define PUT_HILO_WORD(w, addr) (				\
	*((WORD *)(addr)) = (WORD)(w)					\
	)
#else
	#define PUT_HILO_WORD(w, addr) (				\
	((BYTE *)(addr))[0] = (BYTE)((w) >> 8),				\
	((BYTE *)(addr))[1] = (BYTE)(w)					\
	)
#endif

/*
 * aliases 
 */
#define PUT_LEN(wrd, ptr_to_len) PUT_HILO_WORD(wrd, ptr_to_len)
#define PUT_SOCKET(wrd, ptr_to_socket) PUT_HILO_WORD(wrd, ptr_to_socket)

/*
 *	PUT_HILO_LONG(long, address)
 *
 *	Description:
 *		Given a 4 byte word in host order and an address in memory,
 *		store the word in memory in hilo order.
 *		Address may be misaligned.
 *
 *	Purpose:
 *		Included for consistancy.  Actual need not determined.
 *		Most of ipx/router/sap keep net numbers in hilo order
 *		so conversion is rarely needed.
 *
 *	Note:
 *		Beware of side effects in both word and address since
 *		they may each be evaluated four times.
 */

/*
 * if this machine's byte order is hilo and it is not strictly aligned
 * then do a simple store. otherwise do it a byte at a time.
 */
#if defined(HI_LO_MACH_TYPE) && !defined(STRICT_ALIGNMENT)
	#define PUT_HILO_LONG(l, addr) (				\
	*((LONG *)(addr)) = (LONG)(l)					\
	)
#else
	#define PUT_HILO_LONG(l, addr) (				\
	((BYTE *)(addr))[0] = (BYTE)((l) >> 24),			\
	((BYTE *)(addr))[1] = (BYTE)((l) >> 16),			\
	((BYTE *)(addr))[2] = (BYTE)((l) >> 8),				\
	((BYTE *)(addr))[3] = (BYTE)(l)					\
	)
#endif

/*
 *	GET_LOHI_WORD(address)
 *
 *	Description:
 *		Given an address of a 2 byte word in lohi order in memory,
 *		return its value in host order.
 *		Address may be misaligned.
 *
 *	Purpose:
 *		Useful for dealing with length and socket number fields in
 *		ipx headers and fuction fields in sap and rip packets.
 *
 *	Example:
 *		WORD len;
 *		..
 *		len = GET_LOHI_WORD(&ipxpacketptr->length);
 *		if (len > MAX_PACKET_SIZE)
 *			goto BadPacket;
 *		..
 *
 *	Note:
 *		Beware of side effects of addr since it will
 * 		be evaluated twice.
 */

/*
 * if this machine's byte order is lohi and it is not strictly aligned
 * then do a simple fetch. otherwise do it a byte at a time.
 */
#if defined(LO_HI_MACH_TYPE) && !defined(STRICT_ALIGNMENT)
	#define GET_LOHI_WORD(addr) (*(WORD *)(addr))
#else
	#define GET_LOHI_WORD(addr) (					\
	((BYTE *)(addr))[1] << 8 |					\
	((BYTE *)(addr))[0]						\
	) 
#endif

/*
 *	GET_LOHI_LONG(address)
 *
 *	Description:
 *		Given an address of a 4 byte long in lohi order in memory,
 *		return its value in host order.
 *		Address may be misaligned.
 *
 *	Purpose:
 *		Useful for dealing with network numbers in ipx packets
 *		when host order is required.  Typically this is only
 *		when doing compares or other arithmetic operations.  This
 *		is the case because ipx/router/sap usually keep network 
 *		numbers in lohi order so a conversion to host order is
 *		not usually neccessary.
 *
 *	Example:
 *		LONG netnumber;
 *		..
 *		netnumber = GET_LOHI_LONG(&ipxpacketptr->srcnetnumber);
 *		RoutineThatNeedsNetnumberInHostOrder(ipxpacket, netnumber);
 *		..
 *
 *	Note:
 *		Beware of side effects of address since it will
 * 		be evaluated four times.
 */

/*
 * if this machine's byte order is lohi and it is not strictly aligned
 * then do a simple fetch. otherwise do it a byte at a time.
 */
#if defined(LO_HI_MACH_TYPE) && !defined(STRICT_ALIGNMENT)
	#define GET_LOHI_LONG(addr) (*(LONG *)(addr))
#else
	#define GET_LOHI_LONG(addr) (					\
	((BYTE *)(addr))[3] << 24 |					\
	((BYTE *)(addr))[2] << 16 |					\
	((BYTE *)(addr))[1] << 8 |					\
	((BYTE *)(addr))[0]						\
	)
#endif

/*
 *	PUT_LOHI_WORD(word, address)
 *
 *	Description:
 *		Given a 2 byte word in host order and an address in memory,
 *		store the word in memory in lohi order.
 *		Address may be misaligned.
 *
 *	Purpose:
 *		Useful for stuffing the length in an ipx packet and
 *		function in rip and sap packets.
 *
 *	Example:
 *		PUT_LOHI_WORD(2, &routeans->Operation)
 *
 *	Note:
 *		Beware of side effects in both word and address since
 *		they may each be evaluated twice.
 */

/*
 * if this machine's byte order is lohi and it is not strictly aligned
 * then do a simple store. otherwise do it a byte at a time.
 */
#if defined(LO_HI_MACH_TYPE) && !defined(STRICT_ALIGNMENT)
	#define PUT_LOHI_WORD(w, addr) (				\
	*((WORD *)(addr)) = (WORD)(w)					\
	)
#else
	#define PUT_LOHI_WORD(w, addr) (				\
	((BYTE *)(addr))[1] = (BYTE)((w) >> 8),				\
	((BYTE *)(addr))[0] = (BYTE)(w)					\
	)
#endif

/*
 *	PUT_LOHI_LONG(long, address)
 *
 *	Description:
 *		Given a 4 byte word in host order and an address in memory,
 *		store the long in memory in lohi order.
 *		Address may be misaligned.
 *
 *	Purpose:
 *		Included for consistancy.  Actual need not determined.
 *
 *	Note:
 *		Beware of side effects in both word and address since
 *		they may each be evaluated four times.
 */

/*
 * if this machine's byte order is lohi and it is not strictly aligned
 * then do a simple store. otherwise do it a byte at a time.
 */
#if defined(LO_HI_MACH_TYPE) && !defined(STRICT_ALIGNMENT)
	#define PUT_LOHI_LONG(l, addr) (				\
	*((LONG *)(addr)) = (LONG)(l)					\
	)
#else
	#define PUT_LOHI_LONG(l, addr) (				\
	((BYTE *)(addr))[3] = (BYTE)((l) >> 24),			\
	((BYTE *)(addr))[2] = (BYTE)((l) >> 16),			\
	((BYTE *)(addr))[1] = (BYTE)((l) >> 8),				\
	((BYTE *)(addr))[0] = (BYTE)(l)					\
	)
#endif

/*
 *	VALUE_TO_HILO_WORD(word)
 *	WORD_TO_HILO(word)				alias
 *	SOCKET_NUMBER(hostorder_sock)	alias
 *	VALUE_FROM_HILO_WORD(word)		alias
 *
 *	Description:
 *		Given a word in host byte order, return its value in hilo
 *		order. Parameter may be a variable or constant.  
 *
 *	Purpose:
 *		For converting values not in memory (ie constants, register
 * 		variables and expressions) to hilo order.
 *
 *	Examples:
 *		WORD len = VALUE_TO_HILO_WORD(sizeof (struct foobar));
 *		..
 *		..
 *		CIPXOpenThatWantsSocketInHiloOrder(SOCKET_NUMBER(0x455));
 *		..
 *
 *	Note:
 *		Beware of side affects since word may be evaluated twice.
 */
#if defined(HI_LO_MACH_TYPE)
	#define VALUE_TO_HILO_WORD(w) (w)
#else
	#define VALUE_TO_HILO_WORD(w) (					\
	((WORD)w & 0x00ff) << 8 	|				\
	((WORD)w & 0xff00) >> 8						\
	)
#endif

/*
 * aliases.
 */
#define SOCKET_NUMBER(hostorder_sock) 	VALUE_TO_HILO_WORD(hostorder_sock)
#define WORD_TO_HILO(hostorder_sock)	VALUE_TO_HILO_WORD(hostorder_sock)
#define VALUE_FROM_HILO_WORD(l)		VALUE_TO_HILO_WORD(l)

/*
 *	VALUE_TO_HILO_LONG(long)
 *	LONG_TO_HILO(long)				alias
 *	VALUE_FROM_HILO_LONG(long)		alias
 *
 *	Description:
 *		Given a long in host byte order, return its value in
 *		hilo order. Parameter may be a variable or constant.  
 *
 *	Purpose:
 *		For converting values not in memory (ie constants,
 *		register variables and expressions) to hilo order.
 *
 *	Example:
 *		..
 *		LONG internalnetnumberinhilo = VALUE_TO_HILO_LONG(
						netnumberinhostorder);
 *		..
 *
 *	Note:
 *		Beware of side affects since long may be evaluated twice.
 */
#if defined(HI_LO_MACH_TYPE)
	#define VALUE_TO_HILO_LONG(l) (l)
#else
	#define VALUE_TO_HILO_LONG(l) (					\
	((LONG)l & 0x000000ff) << 24 	|				\
	((LONG)l & 0x0000ff00) << 8 	|				\
	((LONG)l & 0x00ff0000) >> 8 	|				\
	((LONG)l & 0xff000000) >> 24					\
	)
#endif

/*
 * aliases.
 */
#define LONG_TO_HILO(hostorder_sock) 	VALUE_TO_HILO_LONG(hostorder_sock)
#define VALUE_FROM_HILO_LONG(l)		VALUE_TO_HILO_LONG(l)

/*
 *	VALUE_TO_LOHI_WORD(word)
 *	WORD_TO_LOHI(word)			alias
 *	VALUE_FROM_LOHI_WORD(word)		alias
 *
 *	Description:
 *		Given a word in host byte order, return its value in lohi order.
 *		Parameter may be a variable or constant.  
 *
 *	Purpose:
 *		For converting values not in memory (ie constants,
 *		register variables and expressions) to lohi order.
 *
 *	Example:
 *		WORD len = VALUE_TO_LOHI_WORD(sizeof (struct foobar));
 *
 *	Note:
 *		Beware of side affects since word may be evaluated twice.
 */
#if defined(LO_HI_MACH_TYPE)
	#define VALUE_TO_LOHI_WORD(w) (w)
#else
	#define VALUE_TO_LOHI_WORD(w) (					\
	((WORD)w & 0x00ff) << 8 	|				\
	((WORD)w & 0xff00) >> 8						\
	)
#endif

/*
 * aliases.
 */
#define WORD_TO_LOHI(w) 	VALUE_TO_LOHI_WORD(w)
#define VALUE_FROM_LOHI_WORD(l) VALUE_TO_LOHI_WORD(l)


/*
 *	VALUE_TO_LOHI_LONG(long)
 *	LONG_TO_LOHI(long)				alias
 *	VALUE_FROM_LOHI_LONG(long)		alias
 *
 *	Description:
 *		Given a long in host byte order, return its value in lohi order.
 *		Parameter may be a variable or constant.  
 *
 *	Purpose:
 *		For converting values not in memory (ie constants,
 *		register variables and expressions) to hilo order.
 *
 *	Example:
 *		..
 *		LONG internalnetnumberinlohi = VALUE_TO_LOHI_LONG(
						netnumberinhostorder);
 *		..
 *
 *	Note:
 *		Beware of side affects since long may be evaluated twice.
 */
#if defined(LO_HI_MACH_TYPE)
	#define VALUE_TO_LOHI_LONG(l) (l)
#else
	#define VALUE_TO_LOHI_LONG(l) (					\
	((LONG)l & 0x000000ff) << 24 	|				\
	((LONG)l & 0x0000ff00) << 8 	|				\
	((LONG)l & 0x00ff0000) >> 8 	|				\
	((LONG)l & 0xff000000) >> 24					\
	)
#endif

/*
 * aliases.
 */
#define LONG_TO_LOHI(l)		VALUE_TO_LOHI_LONG(l)
#define VALUE_FROM_LOHI_LONG(l) VALUE_TO_LOHI_LONG(l)

/*
 *	COPY_TO_HILO_WORD(src, dest)
 *	COPY_TO_LOHI_WORD(src, dest)
 *	COPY_TO_HILO_LONG(src, dest)
 *	COPY_TO_LOHI_LONG(src, dest)
 *
 *  aliases 
 *	COPY_FROM_HILO_WORD(src, dest)
 *	COPY_FROM_LOHI_WORD(src, dest)
 *	COPY_FROM_HILO_LONG(src, dest)
 *	COPY_FROM_LOHI_LONG(src, dest)
 *
 *	Description:
 *		Given the src address of a WORD or LONG in host order copy its 
 *		contents to the dest address in HILO or LOHI order.
 *
 *	Purpose:
 *		Useful for copying from one address to another when one or both
 * 		addresses may be unaligned.  
 *
 *	Note:
 *		On machines where no swapping is needed this turns into
 *		a byte copy. Notice that the COPY_TO and COPY_FROM areidentical.
 */
#if defined(HI_LO_MACH_TYPE)
	#define COPY_TO_HILO_WORD(src,dest) COPY_2BYTE(src,dest)
	#define COPY_TO_HILO_LONG(src,dest) COPY_4BYTE(src,dest)
	#define COPY_TO_LOHI_WORD(src,dest) {				\
	((BYTE *)(dest))[0] = ((BYTE *)(src))[1];			\
	((BYTE *)(dest))[1] = ((BYTE *)(src))[0];			\
	}

	#define COPY_TO_LOHI_LONG(src,dest) {				\
	((BYTE *)(dest))[0] = ((BYTE *)(src))[3];			\
	((BYTE *)(dest))[1] = ((BYTE *)(src))[2];			\
	((BYTE *)(dest))[2] = ((BYTE *)(src))[1];			\
	((BYTE *)(dest))[3] = ((BYTE *)(src))[0];			\
	}
#else		/* LO_HI_MACH_TYPE */
	#define COPY_TO_LOHI_WORD(src,dest) COPY_2BYTE(src,dest)
	#define COPY_TO_LOHI_LONG(src,dest) COPY_4BYTE(src,dest)
	#define COPY_TO_HILO_WORD(src,dest) {				\
	((BYTE *)(dest))[0] = ((BYTE *)(src))[1];			\
	((BYTE *)(dest))[1] = ((BYTE *)(src))[0];			\
	}

	#define COPY_TO_HILO_LONG(src,dest) {				\
	((BYTE *)(dest))[0] = ((BYTE *)(src))[3];			\
	((BYTE *)(dest))[1] = ((BYTE *)(src))[2];			\
	((BYTE *)(dest))[2] = ((BYTE *)(src))[1];			\
	((BYTE *)(dest))[3] = ((BYTE *)(src))[0];			\
	}
#endif

/*
 * aliases.
 */
#define COPY_FROM_HILO_WORD(src, dest)  COPY_TO_HILO_WORD(src,dest)
#define COPY_FROM_LOHI_WORD(src, dest)  COPY_TO_LOHI_WORD(src,dest)
#define COPY_FROM_HILO_LONG(src, dest)  COPY_TO_HILO_LONG(src,dest)
#define COPY_FROM_LOHI_LONG(src, dest)  COPY_TO_LOHI_LONG(src,dest)

/****************************************************************************/
/*
 *	HOST_TO_HILO_WORD(addr)
 *	HOST_TO_LOHI_WORD(addr)
 *	HOST_TO_HILO_LONG(addr)
 *	HOST_TO_LOHI_LONG(addr)
 *
 *  aliases
 *	HOST_FROM_HILO_WORD(addr)
 *	HOST_FROM_LOHI_WORD(addr)
 *	HOST_FROM_HILO_LONG(addr)
 *	HOST_FROM_LOHI_LONG(addr)
 *
 *	Description:
 *		Given the address of a WORD or LONG in host order 
 *		copy its contents to the same address in HILO or LOHI order.
 *
 *	Purpose:
 *		Useful for assuring the correct endianess of a value found
 *		at addr. addr may be misaligned.
 *
 *	Note:
 *		On machines where no swapping is needed no action is taken.
 */
#if defined(HI_LO_MACH_TYPE)
	#define HOST_TO_HILO_WORD(addr) 		/* Nothing */
	#define HOST_TO_HILO_LONG(addr) 		/* Nothing */
	#define HOST_TO_LOHI_WORD(addr) {				\
		BYTE temp;						\
		temp = ((BYTE *)(addr))[0];				\
		((BYTE *)(addr))[0] = ((BYTE *)(addr))[1];		\
		((BYTE *)(addr))[1] = temp;				\
	}

	#define HOST_TO_LOHI_LONG(addr) {				\
		BYTE temp;						\
		temp = ((BYTE *)(addr))[0];				\
		((BYTE *)(addr))[0] = ((BYTE *)(addr))[3];		\
		((BYTE *)(addr))[3] = temp;				\
		temp = ((BYTE *)(addr))[1];				\
		((BYTE *)(addr))[1] = ((BYTE *)(addr))[2];		\
		((BYTE *)(addr))[2] = temp;				\
	}
#else		/* LO_HI_MACH_TYPE */
	#define HOST_TO_LOHI_WORD(addr)			/* Nothing */
	#define HOST_TO_LOHI_LONG(addr)			/* Nothing */
	#define HOST_TO_HILO_WORD(addr) {				\
		BYTE temp;						\
		temp = ((BYTE *)(addr))[0];				\
		((BYTE *)(addr))[0] = ((BYTE *)(addr))[1];		\
		((BYTE *)(addr))[1] = temp;				\
	}

#define HOST_TO_HILO_LONG(addr) {					\
		BYTE temp;						\
		temp = ((BYTE *)(addr))[0];				\
		((BYTE *)(addr))[0] = ((BYTE *)(addr))[3];		\
		((BYTE *)(addr))[3] = temp;				\
		temp = ((BYTE *)(addr))[1];				\
		((BYTE *)(addr))[1] = ((BYTE *)(addr))[2];		\
		((BYTE *)(addr))[2] = temp;				\
	}
#endif

/*
 * aliases.
 */
#define HOST_FROM_HILO_WORD(addr) HOST_TO_HILO_WORD(addr)
#define	HOST_FROM_LOHI_WORD(addr) HOST_TO_LOHI_WORD(addr)
#define	HOST_FROM_HILO_LONG(addr) HOST_TO_HILO_LONG(addr)
#define	HOST_FROM_LOHI_LONG(addr) HOST_TO_LOHI_LONG(addr)

/*
 *	ARE_4BYTES_EQUAL(addr1, addr2)
 *	NET_EQUAL(addr1, addr2)			alias
 *
 *	ARE_6BYTES_EQUAL(addr1, addr2)
 *	NODE_EQUAL(addr1, addr2)		alias
 *	HOST_EQUAL(addr1, addr2)		alias
 *
 *	ARE_12BYTES_EQUAL(addr1, addr2)
 *	ADDRESS_EQUAL(addr1, addr2)		alias
 *
 *	Description:
 *		Compare 2, 4, 6 or 12 bytes of misaligned data and return true
 * 		if they are equal.
 *
 *	Purpose:
 *		Useful for comparing ipx net, nodes or complete ipx addresses.
 *
 *	Note:
 *		It is assumed that 2 byte versions are not very
 *		useful.  In these cases usually at least one of the values
 *		will be in variable and a compare can be done by doing
 *		the appropriate "GET" and an ==.
 */

#define ARE_2BYTES_EQUAL(addr1, addr2) (				\
	((BYTE *)(addr1))[0] == ((BYTE *)(addr2))[0] &&			\
	((BYTE *)(addr1))[1] == ((BYTE *)(addr2))[1] 			\
)

#define ARE_4BYTES_EQUAL(addr1, addr2) (				\
	((BYTE *)(addr1))[0] == ((BYTE *)(addr2))[0] &&			\
	((BYTE *)(addr1))[1] == ((BYTE *)(addr2))[1] &&			\
	((BYTE *)(addr1))[2] == ((BYTE *)(addr2))[2] &&			\
	((BYTE *)(addr1))[3] == ((BYTE *)(addr2))[3]			\
)

#define ARE_6BYTES_EQUAL(addr1, addr2) (				\
	((BYTE *)(addr1))[0] == ((BYTE *)(addr2))[0] &&			\
	((BYTE *)(addr1))[1] == ((BYTE *)(addr2))[1] &&			\
	((BYTE *)(addr1))[2] == ((BYTE *)(addr2))[2] &&			\
	((BYTE *)(addr1))[3] == ((BYTE *)(addr2))[3] &&			\
	((BYTE *)(addr1))[4] == ((BYTE *)(addr2))[4] &&			\
	((BYTE *)(addr1))[5] == ((BYTE *)(addr2))[5]			\
)

#define ARE_12BYTES_EQUAL(addr1, addr2) (				\
	((BYTE *)(addr1))[0] == ((BYTE *)(addr2))[0] &&			\
	((BYTE *)(addr1))[1] == ((BYTE *)(addr2))[1] &&			\
	((BYTE *)(addr1))[2] == ((BYTE *)(addr2))[2] &&			\
	((BYTE *)(addr1))[3] == ((BYTE *)(addr2))[3] &&			\
	((BYTE *)(addr1))[4] == ((BYTE *)(addr2))[4] &&			\
	((BYTE *)(addr1))[5] == ((BYTE *)(addr2))[5] &&			\
	((BYTE *)(addr1))[6] == ((BYTE *)(addr2))[6] &&			\
	((BYTE *)(addr1))[7] == ((BYTE *)(addr2))[7] &&			\
	((BYTE *)(addr1))[8] == ((BYTE *)(addr2))[8] &&			\
	((BYTE *)(addr1))[9] == ((BYTE *)(addr2))[9] &&			\
	((BYTE *)(addr1))[10] == ((BYTE *)(addr2))[10] &&		\
	((BYTE *)(addr1))[11] == ((BYTE *)(addr2))[11]			\
)

/*
 * aliases.
 */
#define WORD_EQUAL(addr1, addr2)	ARE_2BYTES_EQUAL(addr1, addr2)
#define NET_EQUAL(addr1, addr2)		ARE_4BYTES_EQUAL(addr1, addr2)
#define LONG_EQUAL(addr1, addr2) 	ARE_4BYTES_EQUAL(addr1, addr2)
#define NODE_EQUAL(addr1, addr2)	ARE_6BYTES_EQUAL(addr1, addr2)
#define HOST_EQUAL(addr1, addr2)	ARE_6BYTES_EQUAL(addr1, addr2)
#define ADDRESS_EQUAL(addr1, addr2)	ARE_12BYTES_EQUAL(addr1, addr2)

/*
 * Alternate definitions for LONG and WORD that prevent unwanted padding
 * in packed structures.  
 *
 * MisalignedLONG and MisalignedWORD are used if the field is
 * misaligned in the structure. Strictly aligned machines will fault
 * if the appropriate portability macro is not used when accessing them.  
 *
 * eg:
 * 		struct Structure{
 *			WORD	w;    		* RISC compilers add 2 *
 *						* bytes of padding here *
 *			LONG	l; 
 *		};
 *
 * 		struct PackedStructure{
 *			WORD	w; 		* no padding *
 *			MisalignedLONG l;	* use macros to access *
 *						* this field *
 *		} PackedStructure;
 *
 *		PUT_LONG(x, &PackedStructure.l);
 *		y = GET_WORD(&PackedStructure.w);
 *
 * PackedLONG and PackedWORD are used if a field is aligned in a structure
 * but the LONG or WORD causes unwanted padding at the end of the structure
 * or if one structure is embedded in another packed structure, the embedded
 * structure may need to be packed to avoid padding in the other structure.
 * PackedLONG and PackedWORD can be accessed on strictly aligned computers
 * by casting their address to either a (LONG *) or (WORD *).
 *
 * eg:    
 *		struct Structure{
 *			LONG l;
 *			WORD w;			* 2 byte pad; sizeof *
 *						* (struct Structure) equals *
 *						* 8, not 6. *
 *		};
 *
 *		struct PackedStructure{
 *			PackedLONG 	l;
 *			WORD 		w;	* no pad, sizeof *
 *						* (PackedStructure) *
 *						* equals 6. *
 *		} PackedStructure; 
 *
 *		*(LONG *)&PackedStructure.l = x;
 *		y = *(WORD *)&PackedStructure.w;
 */

#ifndef MisalignedLONG
	#ifndef STRICT_ALIGNMENT
		#define MisalignedLONG LONG
	#else
		typedef struct {BYTE l[4];} MisalignedLONG;
	#endif
#endif

#ifndef MisalignedWORD
	#ifndef STRICT_ALIGNMENT
		#define MisalignedWORD unsigned short
	#else
		typedef struct{ BYTE w[2];} MisalignedWORD;
	#endif
#endif

#ifndef PackedLONG
	#ifndef STRICT_ALIGNMENT
		#define PackedLONG LONG
	#else
		typedef struct{ BYTE l[4];} PackedLONG;
	#endif
#endif

#ifndef PackedWORD
	#ifndef STRICT_ALIGNMENT
		#define PackedWORD unsigned short
	#else
		typedef struct{ BYTE w[2];} PackedWORD;
	#endif
#endif

#endif /* _IO_MSM_MSMPORTABLE_H */
