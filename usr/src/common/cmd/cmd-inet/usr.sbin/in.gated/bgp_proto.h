#ident	"@(#)bgp_proto.h	1.3"
#ident	"$Header$"

/*
 * Public Release 3
 * 
 * $Id$
 */

/*
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1996, 1997 The Regents of the University of Michigan
 * All Rights Reserved
 *  
 * Royalty-free licenses to redistribute GateD Release
 * 3 in whole or in part may be obtained by writing to:
 * 
 * 	Merit GateDaemon Project
 * 	4251 Plymouth Road, Suite C
 * 	Ann Arbor, MI 48105
 *  
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS OF THE
 * UNIVERSITY OF MICHIGAN AND MERIT DO NOT WARRANT THAT THE
 * FUNCTIONS CONTAINED IN THE SOFTWARE WILL MEET LICENSEE'S REQUIREMENTS OR
 * THAT OPERATION WILL BE UNINTERRUPTED OR ERROR FREE. The Regents of the
 * University of Michigan and Merit shall not be liable for
 * any special, indirect, incidental or consequential damages with respect
 * to any claim by Licensee or any third party arising from use of the
 * software. GateDaemon was originated and developed through release 3.0
 * by Cornell University and its collaborators.
 * 
 * Please forward bug fixes, enhancements and questions to the
 * gated mailing list: gated-people@gated.merit.edu.
 * 
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1990,1991,1992,1993,1994,1995 by Cornell University.
 *     All rights reserved.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * GateD is based on Kirton's EGP, UC Berkeley's routing
 * daemon	 (routed), and DCN's HELLO routing Protocol.
 * Development of GateD has been supported in part by the
 * National Science Foundation.
 * 
 * ------------------------------------------------------------------------
 * 
 * Portions of this software may fall under the following
 * copyrights:
 * 
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms are
 * permitted provided that the above copyright notice and
 * this paragraph are duplicated in all such forms and that
 * any documentation, advertising materials, and other
 * materials related to such distribution and use
 * acknowledge that the software was developed by the
 * University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote
 * products derived from this software without specific
 * prior written permission.  THIS SOFTWARE IS PROVIDED
 * ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef	__BGP_PROTO_H__
#define	__BGP_PROTO_H__		/* nothing */

/*
 *	Protocol definitions for BGP.  The stuff in here is related to
 *	the BGP protocol itself and should be independent of gated.
 */

/*
 * Some basic constants.
 */
#define	BGP_PORT		179	/* Port number to use with BGP */
#define	BGPMAXPACKETSIZE	4096	/* Maximum message size */

/*
 * BGP message types
 */
#define	BGP_OPEN	1		/* open message */
#define	BGP_UPDATE	2		/* update message */
#define	BGP_NOTIFY	3		/* notification message */
#define	BGP_KEEPALIVE	4		/* keepalive message */
#define	BGP_PACKET_MAX	5

/*
 * BGP versions.  We understand three, version 2, version 3 and
 * version 4.  The differences between the version 2 and 3 protocols
 * are slight, more substantial for version 4.
 */
#define	BGP_VERSION_UNSPEC	0
#define	BGP_VERSION_2		2
#define	BGP_VERSION_3		3
#define	BGP_VERSION_4		4
#define	BGP_KNOWN_VERSION(version) \
	((version) >= BGP_VERSION_2 && (version) <= BGP_VERSION_4)
#define	BGP_VERSION_3OR4(v)	((v) == BGP_VERSION_3 || (v) == BGP_VERSION_4)
#define	BGP_VERSION_2OR3(v)	((v) == BGP_VERSION_2 || (v) == BGP_VERSION_3)

/*
 * Minimum length of a BGP message is the length of the header.  If you
 * haven't got this, you haven't got anything.
 */
#define	BGP_HEADER_LEN	19

/*
 * Authentication types.  We only know one type at this point.
 */
#define	BGP_AUTH_NONE	0

/*
 * AS numbers of significance to BGP
 */
#define	BGP_AS_ANON	0
#define	BGP_AS_HIGH	65535

/*
 * BGP message parsing.  We assume no particular alignment anywhere
 * so that we can parse a stream.  Because of this we use no structure
 * overlays.  Instead we use macros which know the packet formats to
 * pull values out of the stream.  This avoids most byte order issues
 * as well.
 */

/*
 * The length of a BGP route in the message (4 bytes currently).
 */
#define	BGP_ROUTE_LENGTH	4

/*
 * Macros to get various length values from the stream.  cp must be a
 * (byte *)
 */
#define	BGP_GET_BYTE(val, cp)	((val) = *(cp)++)

#define	BGP_GET_SHORT(val, cp) \
	do { \
		register unsigned int Xv; \
		Xv = (*(cp)++) << 8; \
		Xv |= *(cp)++; \
		(val) = Xv; \
	} while (0)

#define	BGP_GET_LONG(val, cp) \
	do { \
		register u_long Xv; \
		Xv = (*(cp)++) << 24; \
		Xv |= (*(cp)++) << 16; \
		Xv |= (*(cp)++) << 8; \
		Xv |= *(cp)++; \
		(val) = Xv; \
	} while (0)

#define	BGP_GET_NETLONG(val, cp) \
	do { \
		register u_char *Xvp; \
		u_long Xv; \
		Xvp = (u_char *) &Xv; \
		*Xvp++ = *(cp)++; \
		*Xvp++ = *(cp)++; \
		*Xvp++ = *(cp)++; \
		*Xvp++ = *(cp)++; \
		(val) = Xv; \
	} while (0)

/*
 * Extract the BGP header from the stream.  Note that a pointer to
 * the marker is returned, rather than the marker itself.
 *
 * The header is a 16 byte marker, followed by a 2 byte length and
 * a 1 byte message type code.
 */
#define	BGP_HEADER_MARKER_LEN	16

#define	BGP_GET_HEADER(marker, length, type, cp) \
	do { \
		(marker) = (cp); \
		(cp) += BGP_HEADER_MARKER_LEN; \
		BGP_GET_SHORT((length), (cp)); \
		BGP_GET_BYTE((type), (cp)); \
	} while (0)

#define	BGP_GET_HDRLEN(length, cp) \
	do { \
		register int Xlen; \
		Xlen = ((int)*((cp) + 16)) << 8; \
		Xlen |= (int)*((cp) + 17); \
		(length) = Xlen; \
	} while (0)

#define	BGP_GET_HDRTYPE(type, cp)	((type) = *((cp) + 18))


/*
 * Extract open message data.  We treat version 2 and version 3
 * open messages separately since they are different lengths.
 * The following extracts the version from the current point.
 */
#define	BGP_GET_VERSION(cp)	(*(cp))

/*
 * For version two we have a one byte version, a 2 byte AS number,
 * a 2 byte hold time and a one byte auth code.  This totals 6 bytes.
 */
#define	BGP_OPENV2_MIN_LEN	6
#define	BGP_GET_OPEN_V2(version, as, holdtime, authcode, cp) \
	do { \
		BGP_GET_BYTE((version), (cp)); \
		BGP_GET_SHORT((as), (cp)); \
		BGP_GET_SHORT((holdtime), (cp)); \
		BGP_GET_BYTE((authcode), (cp)); \
	} while (0)

/*
 * Version three is nearly the same, with the addition of a 4 byte
 * ID field after the hold time.
 */
#define	BGP_OPENV3_MIN_LEN	10
#define	BGP_GET_OPEN_V3(version, as, holdtime, id, authcode, cp) \
	do { \
		BGP_GET_BYTE((version), (cp)); \
		BGP_GET_SHORT((as), (cp)); \
		BGP_GET_SHORT((holdtime), (cp)); \
		BGP_GET_NETLONG((id), (cp)); \
		BGP_GET_BYTE((authcode), (cp)); \
	} while (0)

/*
 * Version four is identical to version three.  Use the version 3 macro.  The
 * code mostly knows version 3 and version 4 are identical, anyway.
 */
#define	BGP_OPENV4_MIN_LEN	(BGP_OPENV3_MIN_LEN)
#define	BGP_GET_OPEN_V4(version, as, holdtime, id, authcode, cp) \
	  BGP_GET_OPEN_V3((version), (as), (holdtime), (id), (authcode), (cp))

/*
 * The overall minimum length for all open messages
 */
#define	BGP_OPEN_MIN_LEN	BGP_OPENV2_MIN_LEN


/*
 * The update message is essentially a 2 byte length for the attribute
 * data, followed by the attribute buffer, followed by a list of
 * networks.  The attribute data is dealt with by separate code,
 * so all we really need to do is to return the length and a pointer
 * to the attribute buffer.
 */
#define	BGP_UPDATE_MIN_LEN	2	/* make sure we've enough for length */
#define	BGP_GET_UPDATE(attrlen, attrp, cp) \
	do { \
		register int Xlen; \
		Xlen = (*(cp)++) << 8; \
		Xlen |= *(cp)++; \
		(attrlen) = Xlen; \
		(attrp) = (cp); \
		(cp) += Xlen; \
	} while (0)

/*
 * The BGP version 4 update message is more complex, consisting of a
 * 2 byte count of unreachable prefixes, followed by the prefixes, followed
 * by a 2 byte path attribute length, followed by the path attributes,
 * followed by reachable prefixes with those attributes.  The minimum
 * length of this is 4 bytes, and we'll need an extra macro to dig out
 * the unreachable prefix count.
 */
#define	BGP_V4UPDATE_MIN_LEN	4
#define	BGP_GET_V4UPDATE_UNREACH(count, cp)	BGP_GET_SHORT((count), (cp))

/*
 * The size of the version 4 unreachable routes length
 */
#define	BGP_UNREACH_SIZE_LEN	2

/*
 * The size of the total path attribute length in an UPDATE
 */
#define	BGP_ATTR_SIZE_LEN	2

/*
 * The following macro extracts network addresses from the stream.  It
 * is used to decode the end of update messages, and understands that
 * network numbers are stored internally in network byte order.
 */
#define	BGP_GET_ADDR(addr, cp) \
	do { \
		register byte *Xap; \
		Xap = (byte *)&(sock2ip(addr)); \
		*Xap++ = *(cp)++; \
		*Xap++ = *(cp)++; \
		*Xap++ = *(cp)++; \
		*Xap++ = *(cp)++; \
	} while (0)

/*
 * These macros, the alternative to the above, extract BGP4-style
 * address prefixes from the stream.  The bit count is returned
 * by the first, the next couple are for testing (to see if the
 * bitcount is okay and there is space) and the remainder set the
 * address.
 */
#define	BGP_BITCOUNT_LEN	1

#define	BGP_GET_BITCOUNT(bitcount, cp)	((bitcount) = *(cp)++)

#define	BGP_OKAY_BITCOUNT(bc)	((bc) <= 32)

#define	BGP_PREFIX_LEN(bc)	(((bc)+7) >> 3)

#define	BGP_GET_PREFIX(bitcount, addr, cp) \
	do { \
		register byte *Xap; \
		Xap = (byte *)&(sock2ip(addr)); \
		switch (BGP_PREFIX_LEN(bitcount)) { \
		case 1: \
		    *Xap++ = *(cp)++; \
		    *Xap++ = 0; \
		    *Xap++ = 0; \
		    *Xap++ = 0; \
		    break; \
		case 2: \
		    *Xap++ = *(cp)++; \
		    *Xap++ = *(cp)++; \
		    *Xap++ = 0; \
		    *Xap++ = 0; \
		    break; \
		case 3: \
		    *Xap++ = *(cp)++; \
		    *Xap++ = *(cp)++; \
		    *Xap++ = *(cp)++; \
		    *Xap++ = 0; \
		    break; \
		case 4: \
		    *Xap++ = *(cp)++; \
		    *Xap++ = *(cp)++; \
		    *Xap++ = *(cp)++; \
		    *Xap++ = *(cp)++; \
		    break; \
		default: \
		    *Xap++ = 0; \
		    *Xap++ = 0; \
		    *Xap++ = 0; \
		    *Xap++ = 0; \
		    break; \
		} \
	} while (0)

/*
 * The notification message contains a 2 byte code/subcode followed by
 * optional data.  Return the code and subcode.
 */
#define	BGP_NOTIFY_MIN_LEN	2	/* code/subcode pair */
#define	BGP_GET_NOTIFY(code, subcode, cp) \
	do { \
		BGP_GET_BYTE((code), (cp)); \
		BGP_GET_BYTE((subcode), (cp)); \
	} while (0)


/*
 * That is it for incoming messages.  The next set of macroes are used
 * for forming outgoing messages.
 */
#define	BGP_PUT_BYTE(val, cp) 	(*(cp)++ = (byte)(val))

#define	BGP_PUT_SHORT(val, cp) \
	do { \
		register u_short Xv; \
		Xv = (u_short)(val); \
		*(cp)++ = (byte)(Xv >> 8); \
		*(cp)++ = (byte)Xv; \
	} while (0)

#define	BGP_PUT_LONG(val, cp) \
	do { \
		register u_long Xv; \
		Xv = (u_long)(val); \
		*(cp)++ = (byte)(Xv >> 24); \
		*(cp)++ = (byte)(Xv >> 16); \
		*(cp)++ = (byte)(Xv >>  8); \
		*(cp)++ = (byte)Xv; \
	} while (0)

#define	BGP_PUT_NETLONG(val, cp) \
	do { \
		register u_char *Xvp; \
		u_long Xv = (u_long)(val); \
		Xvp = (u_char *)&Xv; \
		*(cp)++ = *Xvp++; \
		*(cp)++ = *Xvp++; \
		*(cp)++ = *Xvp++; \
		*(cp)++ = *Xvp++; \
	} while (0)


/*
 * During normal message formation the header is inserted after
 * the rest of the message is fully formed.  The following just
 * leaves space for the header.
 */
#define	BGP_SKIP_HEADER(cp)    ((cp) += BGP_HEADER_LEN)

/*
 * The following are used to insert the length and type into the
 * header given a pointer to the beginning of the message.
 */
#define	BGP_PUT_HDRLEN(length, cp) \
	do { \
		register u_short Xlen; \
		Xlen = (u_short)(length); \
		*((cp) + 16) = (byte)(Xlen >> 8); \
		*((cp) + 17) = (byte)Xlen; \
	} while (0)

#define BGP_PUT_HDRTYPE(type, cp)	(*((cp) + 18) = (byte)(type))

/*
 * Output a version 2 open message.
 */
#define	BGP_PUT_OPEN_V2(as, holdtime, authcode, cp) \
	do { \
		BGP_PUT_BYTE(BGP_VERSION_2, (cp)); \
		BGP_PUT_SHORT((as), (cp)); \
		BGP_PUT_SHORT((holdtime), (cp)); \
		BGP_PUT_BYTE((authcode), (cp)); \
	} while (0)

/*
 * Version three and four are the same, with the addition of a 4 byte
 * ID field after the hold time to version two.
 */
#define	BGP_PUT_OPEN(version, as, holdtime, id, authcode, cp) \
	do { \
		BGP_PUT_BYTE((version), (cp)); \
		BGP_PUT_SHORT((as), (cp)); \
		BGP_PUT_SHORT((holdtime), (cp)); \
		BGP_PUT_NETLONG((id), (cp)); \
		BGP_PUT_BYTE((authcode), (cp)); \
	} while (0)

/*
 * The attributes in the update message are another case where we
 * must insert the data before inserting the length.  This just
 * skips the length field, it will be filled in later.
 */
#define	BGP_SKIP_ATTRLEN(cp)	((cp) += BGP_UPDATE_MIN_LEN)


/*
 * The following puts a network address into the buffer in the
 * form a BGP update message would like.  We know the address
 * is in network byte order already.
 */
#define	BGP_PUT_ADDR(addr, cp) \
	do { \
		register byte *Xap; \
		Xap = (byte *)&(sock2ip(addr)); \
		*(cp)++ = *Xap++; \
		*(cp)++ = *Xap++; \
		*(cp)++ = *Xap++; \
		*(cp)++ = *Xap++; \
	} while (0)

/*
 * The following puts an address prefix into the buffer in the
 * format BGP4 update messages like it, and is the alternative to
 * the above.
 */
#define	BGP_PUT_PREFIX(bitcount, addr, cp) \
	do { \
		register byte *Xap; \
		register byte Xbc = (byte)(bitcount); \
		*(cp)++ = Xbc; \
		if (Xbc != 0) { \
			Xap = (byte *)&(sock2ip(addr)); \
			switch((unsigned) (Xbc + 7) >> 3) { \
			case 4: \
				*(cp)++ = *Xap++; \
			case 3: \
				*(cp)++ = *Xap++; \
			case 2: \
				*(cp)++ = *Xap++; \
			case 1: \
				*(cp)++ = *Xap++; \
			default: \
				break; \
			} \
		} \
	} while (0)

/*
 * The notification message contains a 2 byte code/subcode followed by
 * optional data.  Insert the code/subcode and optional data, if any.
 * Note that we evaluate macro arguments more than once here.
 */
#define	BGP_PUT_NOTIFY(code, subcode, datalen, data, cp) \
	do { \
		BGP_PUT_BYTE((code), (cp)); \
		BGP_PUT_BYTE((subcode), (cp)); \
		if ((datalen) > 0) { \
			register int Xi = (datalen); \
			register byte *Xdp = (byte *)(data); \
			while (Xi-- > 0) \
				*(cp)++ = *Xdp++; \
		} \
	} while (0)


/*
 * BGP error processing is a little tedious.  The following are
 * the error codes/subcodes we use.
 */
#define	BGP_ERR_HEADER		1	/* message header error */
#define	BGP_ERR_OPEN		2	/* open message error */
#define	BGP_ERR_UPDATE		3	/* update message error */
#define	BGP_ERR_HOLDTIME	4	/* hold timer expired */
#define	BGP_ERR_FSM		5	/* finite state machine error */
#define	BGP_CEASE		6	/* cease message (not an error) */

/*
 * The unspecified subcode is sent when we don't have anything better.
 */
#define	BGP_ERR_UNSPEC		0

/*
 * These subcodes are sent with header errors
 */
#define	BGP_ERRHDR_UNSYNC	1	/* loss of connection synchronization */
#define	BGP_ERRHDR_LENGTH	2	/* bad length */
#define	BGP_ERRHDR_TYPE		3	/* bad message type */

/*
 * These are used with open errors
 */
#define	BGP_ERROPN_VERSION	1	/* a version we don't do */
#define	BGP_ERROPN_AS		2	/* something wrong with the AS */
#define	BGP_ERROPN_BGPID	3	/* we don't like BGP ID (V.3 only) */
#define	BGP_ERROPN_AUTHCODE	4	/* unsupported auth code */
#define	BGP_ERROPN_AUTH		5	/* authentication failure */
#define	BGP_ERROPN_BADHOLDTIME	6	/* don't like the hold time */

/*
 * These are used with update errors.  N.B. the path support code also
 * knows these errors.  The lists should be kept in sync.
 */
#define	BGP_ERRUPD_ATTRLIST	1	/* screwed up attribute list */
#define	BGP_ERRUPD_UNKNOWN	2	/* don't know well known attribute */
#define	BGP_ERRUPD_MISSING	3	/* missing well known attribute */
#define	BGP_ERRUPD_FLAGS	4	/* attribute flags error */
#define	BGP_ERRUPD_LENGTH	5	/* attribute length error */
#define	BGP_ERRUPD_ORIGIN	6	/* origin given is bad */
#define	BGP_ERRUPD_ASLOOP	7	/* routing loop in AS path */
#define	BGP_ERRUPD_NEXTHOP	8	/* next hop screwed up */
#define	BGP_ERRUPD_OPTATTR	9	/* error with optional attribute */
#define	BGP_ERRUPD_BADNET	10	/* bad network */
#define	BGP_ERRUPD_ASPATH	11	/* screwed up AS path structure */

/*
 * Protocol States
 */
#define	BGPSTATE_IDLE		1	/* idle state - ignore everything */
#define	BGPSTATE_CONNECT	2	/* connect state - trying to connect */
#define BGPSTATE_ACTIVE		3	/* waiting for a connection */
#define	BGPSTATE_OPENSENT	4	/* open packet has been sent */
#define	BGPSTATE_OPENCONFIRM	5	/* waiting for a keepalive or notify */
#define	BGPSTATE_ESTABLISHED	6	/* connection has been established */

/*
 * Events
 */
#define	BGPEVENT_START		1	/* Start */
#define	BGPEVENT_STOP		2	/* Stop */
#define	BGPEVENT_OPEN		3	/* Transport connection open */
#define	BGPEVENT_CLOSED		4	/* Transport connection closed */
#define	BGPEVENT_OPENFAIL	5	/* Transport connection open failed */
#define	BGPEVENT_ERROR		6	/* Transport error */
#define	BGPEVENT_CONNRETRY	7	/* Connection retry timer expired */
#define	BGPEVENT_HOLDTIME	8	/* Holdtime expired */
#define	BGPEVENT_KEEPALIVE	9	/* KeepAlive timer */
#define	BGPEVENT_RECVOPEN	10	/* Receive Open message */
#define	BGPEVENT_RECVKEEPALIVE	11	/* Receive KeepAlive message */
#define	BGPEVENT_RECVUPDATE	12	/* Receive Update message */
#define	BGPEVENT_RECVNOTIFY	13	/* Receive Notification message */

#endif				/* __BGP_PROTO_H__ */
