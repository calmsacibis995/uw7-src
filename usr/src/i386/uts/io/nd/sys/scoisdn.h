#ifndef _IO_ND_SYS_SCOISDN_H  /* wrapper symbol for kernel use */
#define _IO_ND_SYS_SCOISDN_H  /* subject to change without notice */

#ident "@(#)scoisdn.h	29.2"
/*
 * File scoisdn.h
 *
 *      Copyright (C) The Santa Cruz Operation, 1995-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 *
 * This interface specification should be included by all
 * applications and drivers supporting ISDN.
 */


/*
 * Here we do a little, very little, for portability.  If the
 * compiler does not act as we expect, we could at least hope
 * for a fatal error.
 *
 * It is further assumed that:
 *	o All values are least significant octet first (little endian).
 *	o Bit fields assign the low bits first.
 *		u_int	foo :1,		This should be bit 0
 *			bar :1,		This should be bit 1
 *			    :0;		The high bits are unused
 *
 * The structures here will use our own defined types so that references
 * to the CAPI spec are easier and so that if conversion is necessary,
 * all instances can easily be found.
 */

    typedef unsigned char	isdnU8_t;
    typedef unsigned char	isdnBfU8_t;	/* Must support bit fields */
    typedef unsigned char	isdnByte_t;
    typedef unsigned char	isdnOctet_t;

    typedef unsigned short	isdnU16_t;
    typedef unsigned short	isdnWord_t;

    typedef unsigned int	isdnU32_t;
    typedef unsigned int	isdnBfU32_t;	/* Must support bit fields */

    typedef unsigned long	isdnDword_t;

/*
 * The ISDN message header
 * 
 * Drivers and applications must use this header containing the CAPI
 * message header prepended with the SCO DLPI primitive.  The DLPI
 * primitive for ISDN is always set to DL_ISDN_MSG.
 * 
 */

typedef struct isdn_msg_hdr
{
	isdnDword_t	DL_prim;
	isdnWord_t	Length;
	isdnWord_t	AppID;
	isdnByte_t	Cmd;
	isdnByte_t	SubCmd;
	isdnWord_t	MsgNum;
} isdn_msg_hdr_t;



/*
 * # # # # # # # # # # # # # # The ISDN ioctls # # # # # # # # # # # # # #
 */

#define ISDN_IOC		('C' << 8)

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * ISDN_REGISTER		ioctl()
 *
 * An application can register at SCO-ISDN by opening the device
 * (/dev/netx) and issuing the relevant parameters via the system
 * call ioctl() to the opened device.  Note that the result of this
 * operation is a file handle, not an application ID.  So in UNIX
 * environment the application ID included in CAPI messages will
 * not be used to identify ISDN applications.  The only valid
 * handle between the SCO-ISDN kernel driver and the application
 * based on a system call level interface is a UNIX file handle.
 * To release from SCO-ISDN, an application just has to close the
 * opened device.
 *
 * MDI ISDN drivers maintain application IDs internally within
 * the kernel, but this is transparent to applications.
 *
 * This operation is realized using ioctl().  The caller must supply
 * a struct isdn_register_params in struct strioctl ic_dp and ic_len.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

#define ISDN_REGISTER		(ISDN_IOC | 0x01)

typedef struct isdn_register_params
{
    isdnU16_t	ApplId;		/* application identifier	*/
    isdnU32_t	level3cnt;	/* No. of simultaneous user data connections */
    isdnU32_t	datablkcnt;	/* No. of buffered data messages */
    isdnU32_t	datablklen;	/* Size of buffered data messages */
} isdn_register_params_t;

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * ISDN_RELEASE		ioctl()
 *
 * This operation is only for an MDI ISDN driver.  Applications do not
 * use it.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

#define ISDN_RELEASE		(ISDN_IOC | 0x02)

typedef struct isdn_release
{
    isdnU16_t	ApplId;		/* application identifier	*/
} isdn_release_t;

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * ISDN_GET_MANUFACTURER	ioctl()
 *
 * With this operation the application determines the manufacturer
 * identification of SCO-ISDN.  The offered buffer must have a size
 * of at least 64 bytes.  SCO-ISDN copies the identification string,
 * coded as a zero terminated ASCII string, to this buffer.
 *
 * This operation is realized using ioctl().  The caller must supply
 * a buffer in struct strioctl ic_dp and ic_len.  The manufacturer
 * identification is transferred to the given buffer.  The string is
 * always zero-terminated.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

#define ISDN_GET_MANUFACTURER	(ISDN_IOC | 0x06)

#define ISDN_MIN_MFGR_BUF	64

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * ISDN_GET_VERSION		ioctl()
 *
 * With this function the application determines the version of
 * SCO-ISDN as well as an internal revision number.  The offered
 * buffer must have a size of sizeof(isdn_version_t).
 *
 * This operation is realized using ioctl().  The caller must supply
 * a struct isdn_version in struct strioctl ic_dp and ic_len.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

#define ISDN_GET_VERSION	(ISDN_IOC | 0x07)

typedef struct isdn_version
{
    isdnU32_t	isdnmajorver;	/* SCO-ISDN major version */
    isdnU32_t	isdnminorver;	/* SCO-ISDN minor version */
    isdnU32_t	mfgrmajorver;	/* manufacturer-specific major number */
    isdnU32_t	mfgrminorver;	/* manufacturer-specific minor number */
} isdn_version_t;

#define ISDN_MAJOR_VERSION	0x02
#define ISDN_MINOR_VERSION	0x00

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * ISDN_GET_SERIAL_NUMBER	ioctl()
 *
 * With this operation the application determines the (optional)
 * serial number of SCO-ISDN.  The offered buffer must have a size
 * of sizeof(isdn_serial_t).  SCO-ISDN copies the serial number
 * string to this buffer.  The serial number, coded as a zero
 * terminated ASCII string, represents seven digit number after
 * the function has returned.
 *
 * This operation is realized using ioctl().  The caller must
 * supply a buffer in struct strioctl ic_dp and ic_len.  The
 * serial number consists of up to seven decimal-digit ASCII
 * characters.  It is always zero-terminated.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

#define ISDN_GET_SERIAL_NUMBER	(ISDN_IOC | 0x08)
#define ISDN_SERIAL_BUF_LEN	8

typedef struct isdn_serial
{
    char	SerialNo[ISDN_SERIAL_BUF_LEN];
} isdn_serial_t;

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * ISDN_GET_PROFILE		ioctl()
 *
 * The application uses this function to get the capabilities from
 * SCO-ISDN.  In the allocated buffer SCO-ISDN copies information
 * about implemented features, number of controllers and supported
 * protocols.  CtrlNr contains the controller number for which this
 * information is requested.
 *
 * This operation is realized using ioctl().  The caller must supply
 * a union isdn_profile in struct strioctl ic_dp and ic_len.
 *
 * This function can be extended, so an application has to ignore
 * unknown bits.  SCO-ISDN will set every reserved field to 0.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

#define ISDN_GET_PROFILE	(ISDN_IOC | 0x09)

typedef union isdn_profile
{
    struct
    {
	isdnBfU8_t	CtrlNr;		/* Controller number */
    } request;

    struct
    {
	isdnU16_t	CtlrCnt;	/* number of installed controllers */
	isdnU16_t	BchanCnt;	/* number of supported B-channels */

	/* Global Options */
	isdnBfU32_t	g_IntCtlr   :1,	/* internal controller supported */
			g_ExtEqpt   :1,	/* external equipment supported */
			g_Handset   :1,	/* Handset supported */
			g_DTMF      :1,	/* DTMF supported */
				    :0;	/* Reserved */

	/* B1 protocols support; Physical layer and framing */
	isdnBfU32_t	b1_HDLC64   :1,	/* 64 kBit/s with HDLC framing */
			b1_Trans64  :1,	/* 64 kBit/s bit transparent */
			b1_V110asy  :1,	/* V.110 asynchronous start/stop */
			b1_V110sync :1,	/* V.110 synchronous HDLC */
			b1_T30      :1,	/* T.30 modem for fax group 3 */
			b1_HDLCi64  :1,	/* 64 kBit/s inverted HDLC */
			b1_Trans56  :1,	/* 56 kBit/s bit transparent */
				    :0;	/* Reserved */

	/* B2 protocol support; Data link layer protocol */
	isdnBfU32_t	b2_ISO7776  :1,	/* ISO 7776 (X.75 SLP) */
			b2_Trans    :1,	/* Transparent */
			b2_SDLC     :1,	/* SDLC */
			b2_LAPD     :1,	/* LAPD Q.921 for D channel X.25 */
			b2_T30      :1,	/* T.30 for fax group 3 */
			b2_PPP      :1,	/* Point to Point Protocol */
			b2_TransNE  :1,	/* Transparent ignoring errors */
				    :0;	/* Reserved */

	/* B3 protocol support; Network layer protocol */
	isdnBfU32_t	b3_Trans    :1,	/* Transparent */
			b3_T90NL    :1,	/* T.90NL w/ compatibility to T.70NL */
			b3_ISO8208  :1,	/* ISO 8208 (X.25 DTE-DTE) */
			b3_X25DCE   :1,	/* X.25 DCE */
			b3_T30      :1,	/* T.30 for fax group 3 */
				    :0;	/* Reserved */

	isdnU32_t	isdn_reserved[6];
	isdnU32_t	mfgr_specific[5];
    } reply;
    unsigned char	buf[64];

} isdn_profile_t;

/*
 * # # # # # # # # # # # # # # The ISDN Messages # # # # # # # # # # # # #
 */

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * ISDN_PUT_MESSAGE		putmsg()
 *
 * With this operation the application transfers a message to
 * SCO-ISDN.  The application identifies itself with an
 * application identification number.
 *
 * To transfer a message from an application to SCO-ISDN
 * driver and the controller behind, the system call putmsg() is
 * used.  The application puts SCO-ISDN message and parameter data
 * into the ctl part of the putmsg() call. The data portion
 * of the message ISDN_DATA_B3.ISDN_REQ have to be stored in the
 * data part of putmsg().
 *
 * In order to facilitate future extensions of this standard, messages
 * containing additional parameters shall be treated as valid messages.
 * SCO-ISDN implementations and applications shall ignore all
 * additional parameters.
 *
 *
 * ISDN_GET_MESSAGE		getmsg()
 *
 * With this operation the application retrieves a message from
 * SCO-ISDN.  The application retrieves all messages associated
 * with the corresponding file descriptor from operation ISDN_REGISTER.
 *
 * To receive a message from SCO-ISDN the application uses
 * the system call getmsg().  The application has to supply sufficient
 * buffers for receiving the ctl and data parts of the message.
 *
 * To receive a message from SCO-ISDN the application uses
 * the system call getmsg().
 *
 *
 * NOTE:
 *
 * The CAPI 2.0 specification defines many of the message parameters
 * as variable length structures.
 *
 * WARNING: 
 *
 * CAPI 2.0 parameters defined as structures are variable length with
 * a length byte preceeding the contents of the structure.  When the
 * length field is zero, the structure is empty.
 *
 * CAPI 2.0 parameters defined as scaler types do not have a length
 * field.
 *
 * The parameters described below in C do not declare the byte length
 * field for structures.  Therefore, they cannot be used as exact
 * templates for real CAPI parameters when it is a structure.  Please
 * use the declarations as a guide, not as an implementation.
 *
 * Scaler parameter declarations can be used in an implementation.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * Subcommands
 *
 *              Application
 *  ------------------------------------
 *     |      ^            ^      |
 *     |      |            |      |
 *     | REQ  | CONF       | IND  | RESP
 *     |      |            |      |
 *     V      |            |      V
 *  ------------------------------------
 *                Driver
 */

#define ISDN_REQ	0x80	/* Request initiation */
#define ISDN_CONF	0x81	/* Local confirmation of request */

#define ISDN_IND	0x82	/* Indication of request */
#define ISDN_RESP	0x83	/* Response to indication */

/*
 * Some types of messages are not supported.  We define a
 * dummy structure of the expected name to detect a definition
 * of these unsupported types.
 */

#define UNUSED_MSG_TYPE(x)	typedef struct { char dummy; } x

/*
 * Additional info
 * This type is used by:
 *	ISDN_ALERT.ISDN_REQ
 *	ISDN_CONNECT.ISDN_REQ
 *	ISDN_CONNECT.ISDN_IND
 *	ISDN_CONNECT.ISDN_RESP
 *	ISDN_DISCONNECT.ISDN_REQ
 *	ISDN_INFO.ISDN_REQ
 */

typedef struct isdnAddInfo
{
    struct
    {
	isdnWord_t	channel;
    } BchanInfo;

    struct
    {
	void	*dummy;	/* ## Q.931 */
    } keypadFacility;

    struct		/* Per Q.931 4.5.27 */
    {
	void	*dummy;	/* ## Q.931 */
    } userUserData;

    struct
    {
	void	*dummy;	/* ## Q.931 */
    } facilityData;
} isdnAddInfo_t;

/*
 * Physical layer and framing protocol
 * This type is used by:
 *	isdnBproto_t
 */

typedef isdnWord_t isdnB1proto_t;

    /* Values for isdnB1proto_t */
#define B1PROTO_HDLC	0x0000	/* 64 kBit/s with HDLC framing */
#define B1PROTO_TRANS	0x0001	/* 64 kBit/s transparent byte framing */
#define B1PROTO_V110ASY	0x0002	/* V.110 async start/stop framing */
#define B1PROTO_V110SY	0x0003	/* v.110 sync with HDLC framing */
#define B1PROTO_T30	0x0004	/* T.30 modem for fax group 3 */
#define B1PROTO_HDLCINV	0x0005	/* 64 kBit/s with inverted HDLC framing */
#define B1PROTO_56TRANS	0x0006	/* 56 kBits/s transparent byte framing */

/*
 * Data link layer protocol
 * This type is used by:
 *	isdnBproto_t
 */

typedef isdnWord_t isdnB2proto_t;

    /* Values for isdnB2proto_t */
#define B2PROTO_X75SLP	0x0000	/* ISO 7776 (X.25 SLP) */
#define B2PROTO_TRANS	0x0001	/* Transparent */
#define B2PROTO_SDLC	0x0002	/* SDLC */
#define B2PROTO_LAPD	0x0003	/* LAPD (Q.921) for D channel X.25 */
#define B2PROTO_T30	0x0004	/* T.30 for fax group 3 */
#define B2PROTO_PPP	0x0005	/* Point to Point Protocol (PPP) */
#define B2PROTO_ERTRANS	0x0006	/* Transparent ignoring B1 framing errors */

/*
 * Network layer protocol
 * This type is used by:
 *	isdnBproto_t
 */

typedef isdnWord_t isdnB3proto_t;

    /* Values for isdnB3proto_t */
#define B3PROTO_TRANS	0x0000	/* Transparent */
#define B3PROTO_T90NL	0x0001	/* T.90NL with T.70NL compatibility */
#define B3PROTO_X25DTE	0x0002	/* ISO 8208 (X.25 DTE-DTE) */
#define B3PROTO_X25DCE	0x0003	/* X.25 DCE */
#define B3PROTO_T30	0x0004	/* T30 for fax group 3 */

/*
 * B2 protocol mode of operation
 * This type is used by:
 *	isdnBproto_t
 */

typedef isdnByte_t isdnModMode_t;

    /* Values for isdnModMode_t */
#define BPROTO_MODE_NORM	8	/* Normal operation */
#define BPROTO_MODE_EXTENDED	128	/* Extended operation */

/*
 * B protocol selection and configuration.
 *
 * This type is used by:
 *	ISDN_CONNECT.ISDN_REQ
 *	ISDN_CONNECT.ISDN_RESP
 *	ISDN_SELECT_B_PROTOCOL.ISDN_REQ
 */

typedef struct isdnBproto
{
    isdnB1proto_t	B1proto;	/* Physical layer and framing proto */
    isdnB2proto_t	B2proto;	/* Data link layer protocol */
    isdnB3proto_t	B3proto;	/* Network layer protocol */

    struct isdnB1config		/* Physical layer and framing parameters */
    {
	isdnWord_t	rate;
	isdnWord_t	dataBits;
	isdnWord_t	parity;
	isdnWord_t	stopBits;
    } B1config;

    struct isdnB2config		/* Data link layer parameters */
    {
	isdnByte_t	addrA;
	isdnByte_t	addrB;
	isdnModMode_t	moduloMode;	/* Mode of operation */
	isdnByte_t	windowSize;

	struct
	{
		void	*dummy;	/* only with protocol 2, XID response */
	} xid;
    } B2config;

    union isdnB3config		/* Network layer parameters */
    {
	struct			/* Used for protocols 1, 2, 3 */
	{
	    isdnWord_t	lic;		/* Lowest incoming channel */
	    isdnWord_t	hic;		/* Highest incoming channel */
	    isdnWord_t	ltc;		/* Lowest two-way channel */
	    isdnWord_t	htc;		/* Highest two-way channel */
	    isdnWord_t	loc;		/* Lowest outgoing channel */
	    isdnWord_t	hoc;		/* highest outgoing channel */
	    isdnWord_t	moduloMode;	/* Mode of operation */
	    isdnWord_t	windowSize;
	} P123;

	struct			/* Used for protocol 4 */
	{
	    isdnWord_t	resolution;
	    isdnWord_t	format;
		struct
		{
			void	*dummy;	/* ID of the calling station */
		} station_id;
		struct
		{
			void	*dummy;	/* headline sent on each fax page */
		} headline;
	} FAX;
    } B3config;

} isdnBproto_t;

typedef struct isdnB1config isdnB1config_t;
typedef struct isdnB2config isdnB2config_t;
typedef union isdnB3config isdnB3config_t;

/*
 * Bearer Capability
 * This type is used by:
 *	ISDN_CONNECT.ISDN_REQ
 *	ISDN_CONNECT.ISDN_IND
 * This type is described by Q.931 4.5.5
 */

typedef struct isdnBC
{
    void	*dummy;	/* ## Q.931 */
} isdnBC_t;

/*
 * Called Party Number
 * This type is used by:
 *	ISDN_CONNECT.ISDN_REQ
 *	ISDN_CONNECT.ISDN_IND
 *	## ISDN_INFO.ISDN_REQ
 */

#define MAX_NUMBER_DIGITS	32	/* should be big enough */

typedef struct isdnCalledNbr
{
    isdnByte_t	type;				/* Num type and num plan id */
    isdnByte_t	number[MAX_NUMBER_DIGITS];	/* Number digits */
} isdnCalledNbr_t;

/*
 * Called Party SubAddress
 * This type is used by:
 *	ISDN_CONNECT.ISDN_REQ
 *	ISDN_CONNECT.ISDN_IND
 */

typedef struct isdnCalledAdr
{
    isdnByte_t	type;				/* Type of subaddress */
    isdnByte_t	number[MAX_NUMBER_DIGITS];	/* Number digits */
} isdnCalledAdr_t;

/*
 * Calling Party Number
 * This type is used by:
 *	ISDN_CONNECT.ISDN_REQ
 *	ISDN_CONNECT.ISDN_IND
 *	ISDN_LISTEN.ISDN_REQ
 */

typedef struct isdnCallingNbr
{
    isdnByte_t	type;				/* Num type and num plan id */
    isdnByte_t	pres;		/* Presentation and screening indicator */
    isdnByte_t	number[MAX_NUMBER_DIGITS];	/* Number digits */
} isdnCallingNbr_t;

/*
 * Calling Party SubAddress
 * This type is used by:
 *	ISDN_CONNECT.ISDN_REQ
 *	ISDN_CONNECT.ISDN_IND
 *	ISDN_LISTEN.ISDN_REQ
 */

typedef struct isdnCallingAdr
{
    isdnByte_t	type;				/* Type of subaddress */
    isdnByte_t	number[MAX_NUMBER_DIGITS];	/* Number digits */
} isdnCallingAdr_t;

/*
 * Compatibility Information Profile Value
 * This type is used by:
 *	ISDN_CONNECT.ISDN_REQ
 *	ISDN_CONNECT.ISDN_IND
 */

typedef isdnWord_t isdnCIPvalue_t;

    /* Values for isdnCIPvalue_t */
#define ISDN_CIPVAL_NONE	 0	/* No predefined profile */
#define ISDN_CIPVAL_SPEECH	 1	/* Speech */
#define ISDN_CIPVAL_UNRESTRICTED 2	/* Unrestricted digital information */
#define ISDN_CIPVAL_RESTRICTED	 3	/* restricted digital information */
#define ISDN_CIPVAL_3KHZAUDIO	 4	/* 3.1kHz audio */
#define ISDN_CIPVAL_7KHZAUDIO	 5	/* 7.0 kHz audio */
#define ISDN_CIPVAL_VIDEO	 6	/* video */
#define ISDN_CIPVAL_PACKET	 7	/* packet mode */
#define ISDN_CIPVAL_56KRATEADAPT 8	/* 56 kBit/s rate adaptation */
#define ISDN_CIPVAL_UNRESWANN	 9	/* unrestricted digital w/ann */
#define ISDN_CIPVAL_TELEPHONY	16	/* telephony */
#define ISDN_CIPVAL_FAXGROUP23	17	/* fax group 2/3 */
#define ISDN_CIPVAL_FAXGROUP4	18	/* fax group 4 class 1 */
#define ISDN_CIPVAL_TELTXMIXED	19	/* Teletex (basic & mixed) */
#define ISDN_CIPVAL_TELTXPROC	20	/* Teletex (basic & proc) */
#define ISDN_CIPVAL_TELTXBASIC	21	/* Teletex (basic) */
#define ISDN_CIPVAL_VIDEOTEX	22	/* Videotex */
#define ISDN_CIPVAL_TELEX	23	/* Telex */
#define ISDN_CIPVAL_X400	24	/* X.400 message handling*/
#define ISDN_CIPVAL_X200	25	/* X.200 OSI applications */
#define ISDN_CIPVAL_7KTELEPHONY 26	/* 7 kHz Telephony */
#define ISDN_CIPVAL_VIDTELFIRST	27	/* Video Tel F.721, first */
#define ISDN_CIPVAL_VIDTELSEC	28	/* Video Tel F.721, second */

/*
 * Compatibility Information Profile Mask
 * This type is used by:
 *	ISDN_LISTEN.ISDN_REQ
 */

typedef isdnDword_t isdnCIPmask_t;

    /* Bit fields for isdnCIPmask_t */
#define ISDN_CIPMSK_NONE	 0x00000000 /* No connect indicatations */
#define ISDN_CIPMSK_ANY		 0x00000001 /*  [0] any match */
#define ISDN_CIPMSK_SPEECH	 0x00000002 /*  [1] speech */
#define ISDN_CIPMSK_UNRESTRICTED 0x00000004 /*  [2] unrestricted digital */
					    /*      information		 */
#define ISDN_CIPMSK_RESTRICTED	 0x00000008 /*  [3] restricted digital info */
#define ISDN_CIPMSK_3KHZAUDIO	 0x00000010 /*  [4] 3.1 kHz audio */
#define ISDN_CIPMSK_7KHZAUDIO	 0x00000020 /*  [5] 7.0 kHz audio */
#define ISDN_CIPMSK_VIDEO	 0x00000040 /*  [6] video */
#define ISDN_CIPMSK_PACKET	 0x00000080 /*  [7] packet mode */
#define ISDN_CIPMSK_56KRATEADAPT 0x00000100 /*  [8] 56 kBit/s rate adapt */
#define ISDN_CIPMSK_UNRESWANN	 0x00000200 /*  [9] unrestricted digital */
					    /*      with announcments    */
#define ISDN_CIPMSK_TELEPHONY	 0x00010000 /* [16] telephony */
#define ISDN_CIPMSK_FAXGROUP23	 0x00020000 /* [17] fax group 2/3 */
#define ISDN_CIPMSK_FAXGROUP4	 0x00040000 /* [18] fax group 4 class 1 */
#define ISDN_CIPMSK_TELTXMIXED	 0x00080000 /* [19] Teletex (basic & mixed) */
#define ISDN_CIPMSK_TELTXPROC	 0x00100000 /* [20] Teletex (basic & proc) */
#define ISDN_CIPMSK_TELTXBASIC	 0x00200000 /* [21] Teletex (basic) */
#define ISDN_CIPMSK_VIDEOTEX	 0x00400000 /* [22] Videotex */
#define ISDN_CIPMSK_TELEX	 0x00800000 /* [23] Telex */
#define ISDN_CIPMSK_X400	 0x01000000 /* [24] X.400 message handling */
#define ISDN_CIPMSK_X200	 0x02000000 /* [25] X.200 OSI applications */
#define ISDN_CIPMSK_7KTELEPHONY	 0x04000000 /* [26] 7 kHz Telephony */
#define ISDN_CIPMSK_VIDTELFIRST	 0x08000000 /* [27] Video Tel F.721, first */
#define ISDN_CIPMSK_VIDTELSEC	 0x10000000 /* [28] Video Tel F.721, second */

/*
 * Connected Number
 * This type is used by:
 *	ISDN_CONNECT.ISDN_RESP
 *	ISDN_CONNECT_ACTIVE.ISDN_IND
 */

typedef struct isdnConnNbr
{
    isdnByte_t	type;				/* Num type and num plan id */
    isdnByte_t	pres;		/* Presentation and screening indicator */
    isdnByte_t	number[MAX_NUMBER_DIGITS];	/* Number digits */
} isdnConnNbr_t;

/*
 * Connected SubAddress
 * This type is used by:
 *	ISDN_CONNECT.ISDN_RESP
 *	ISDN_CONNECT_ACTIVE.ISDN_IND
 */

typedef struct isdnConnAdr
{
    isdnByte_t	type;				/* Type of subaddress */
    isdnByte_t	number[MAX_NUMBER_DIGITS];	/* Number digits */
} isdnConnAdr_t;

/*
 * Controller
 * This type is used by:
 *	ISDN_CONNECT.ISDN_REQ
 *	ISDN_LISTEN.ISDN_REQ
 *	ISDN_LISTEN.ISDN_CONF
 *	ISDN_MANUFACTURER.ISDN_REQ
 *	ISDN_MANUFACTURER.ISDN_CONF
 *	ISDN_MANUFACTURER.ISDN_IND
 *	ISDN_MANUFACTURER.ISDN_RESP
 *	ISDN_FACILITY.ISDN_REQ
 *	ISDN_FACILITY.ISDN_CONF
 *	ISDN_FACILITY.ISDN_IND
 *	ISDN_FACILITY.ISDN_RESP
 */

typedef isdnDword_t isdnCtrlr_t;

    /* Bit masks for isdnCtrlr_t */
#define ISDN_CTLRMSK_UNUSED	0xffffff00	/* Reserved, set to zero */
#define ISDN_CTLRMSK_EXT	0x00000080	/* 1=External, 0=Internal */
#define ISDN_CTLRMSK_CTLR	0x0000007f	/* Controller */

/*
 * Data
 * This type is used by:
 *	ISDN_DATA_B3.ISDN_REQ
 *	ISDN_DATA_B3.ISDN_IND
 */

typedef isdnDword_t isdnData_t;

/*
 * Data Length
 * This type is used by:
 *	ISDN_DATA_B3.ISDN_REQ
 *	ISDN_DATA_B3.ISDN_IND
 */

typedef isdnWord_t isdnDataLen_t;

/*
 * Data Handle
 * This type is used by:
 *	ISDN_DATA_B3.ISDN_REQ
 *	ISDN_DATA_B3.ISDN_IND
 */

typedef isdnWord_t isdnHandle_t;	/* ## Used? file handle?? */

/*
 * Facility Selector
 * This type is used by:
 *	ISDN_FACILITY.ISDN_REQ
 *	ISDN_FACILITY.ISDN_CONF
 *	ISDN_FACILITY.ISDN_IND
 *	ISDN_FACILITY.ISDN_RESP
 */

typedef isdnWord_t isdnFacility_t;

    /* Values for isdnFacility_t */
#define ISDN_FACILITY_HANDSET	0x0000	/* Handset Support */
#define ISDN_FACILITY_DTMF	0x0001	/* DTMF */

/*
 * Facility Request Parameter
 * This type is used by:
 *	ISDN_FACILITY.ISDN_REQ
 */

typedef struct isdnFRP
{
	isdnWord_t	function;
	isdnWord_t	toneDuration;
	isdnWord_t	gapDuration;
	struct
	{
		isdnByte_t	digits[MAX_NUMBER_DIGITS];
	} DTMFdigits;
} isdnFRP_t;

/*
 * Facility Confirmation Parameter
 * This type is used by:
 *	ISDN_FACILITY.ISDN_CONF
 */

typedef struct isdnFCP
{
    isdnWord_t	info;	/* DTMF information */
} isdnFCP_t;

/*
 * Facility Indication Parameter
 * This type is used by:
 *	ISDN_FACILITY.ISDN_IND
 */

typedef struct isdnFIP
{
    isdnByte_t	digits[MAX_NUMBER_DIGITS];	/* Handset or DTMF digits */
} isdnFIP_t;

/*
 * Flags
 * This type is used by:
 *	ISDN_DATA_B3.ISDN_REQ
 *	ISDN_DATA_B3.ISDN_IND
 */

typedef isdnWord_t isdnFlags_t;

    /* Bit masks for isdnFlags_t */
#define ISDN_DATAFLAG_QUALIFIERBIT	0x0001	/* Qualifier bit */
#define ISDN_DATAFLAG_MOREDATA		0x0002	/* More data bit */
#define ISDN_DATAFLAG_DELIVERYCONF	0x0004	/* Delivery confirmation bit */
#define ISDN_DATAFLAG_EXPEDITEDDATA	0x0008	/* Expedited data */
#define ISDN_DATAFLAG_FRAMINGERROR	0x8000	/* Framing error */

/*
 * High Layer Compatibility
 * This type is used by:
 *	ISDN_CONNECT.ISDN_REQ
 *	ISDN_CONNECT.ISDN_IND
 */

typedef struct isdnHLC
{
    void	*dummy;		/* ## Q.931 */
} isdnHLC_t;

/*
 * Info - Information status code
 * This type is used by:
 *	ISDN_CONNECT.ISDN_CONF
 *	ISDN_DISCONNECT.ISDN_CONF
 *	ISDN_ALERT.ISDN_CONF
 *	ISDN_INFO.ISDN_CONF
 *	ISDN_CONNECT_B3.ISDN_CONF
 *	ISDN_DISCONNECT_B3.ISDN_CONF
 *	ISDN_DATA_B3.ISDN_CONF
 *	ISDN_RESET_B3.ISDN_CONF
 *	ISDN_LISTEN.ISDN_CONF
 *	ISDN_FACILITY.ISDN_CONF
 *	ISDN_SELECT_B_PROTOCOL.ISDN_CONF
 */

typedef isdnWord_t	isdnInfo_t;

    /* Values for class 0x00xx:	Informative values (message was processed) */
#define ISDN_INFOCODE00_OK	0x0000	/* Request accepted */
#define ISDN_INFOCODE00_BADNCPI	0x0001	/* NCPI not supported, ignored */
#define ISDN_INFOCODE00_NOSYS	0x0002	/* flags not supported, ignored */
#define ISDN_INFOCODE00_EXIST	0x0003	/* Alert already sent by another */

    /* Values for class 0x10xx:	ISDN_REGISTER errors */
#define ISDN_INFOCODE10_TMAPPS	0x1001	/* Too many applications */
#define ISDN_INFOCODE10_SBSIZE	0x1002	/* Logical block size too small */
#define ISDN_INFOCODE10_BTLARGE 0x1003	/* Buffer too large */
#define ISDN_INFOCODE10_BTSMALL	0x1004	/* Message buffer too small */
#define ISDN_INFOCODE10_INVALLC 0x1005	/* Invalid max logical connections */
#define ISDN_INFOCODE10_RSVD	0x1006	/* Reserved */
#define ISDN_INFOCODE10_BUSY	0x1007	/* Internal busy */
#define ISDN_INFOCODE10_RESERR	0x1008	/* OS resource error */
#define ISDN_INFOCODE10_NOCAPI	0x1009	/* ISDN not installed */
#define ISDN_INFOCODE10_NOEXT	0x100a	/* Ctlr does not support external */
#define ISDN_INFOCODE10_ONLYEXT	0x100b	/* Ctlr does not support internal */

    /* Values for class 0x11xx:	Message exchange function errors */
#define ISDN_INFOCODE11_BADAPP	0x1101	/* Illegal application */
#define ISDN_INFOCODE11_BADCMD	0x1102	/* Illegal cmd/subcmd/length */
#define ISDN_INFOCODE11_FULL	0x1103	/* queue full */
#define ISDN_INFOCODE11_EMPTY	0x1104	/* queue is empty */
#define ISDN_INFOCODE11_OVFL	0x1105	/* queue overflow */
#define ISDN_INFOCODE11_INVAL	0x1106	/* unknown notification param */
#define ISDN_INFOCODE11_BUSY	0x1107	/* Internal busy */
#define ISDN_INFOCODE11_RESERR	0x1108	/* OS resource error */
#define ISDN_INFOCODE11_NOCAPI	0x1109	/* ISDN not installed */
#define ISDN_INFOCODE11_NOEXT	0x110a	/* Ctlr does not support external */
#define ISDN_INFOCODE11_ONLYEXT	0x110b	/* Ctlr does not support internal */

    /* Values for class 0x20xx:	Resource/coding problems */
#define ISDN_INFOCODE20_ESTATE	0x2001	/* Message not supported in state */
#define ISDN_INFOCODE20_EPLCI	0x2002	/* Illegal Ctlr/PLCI/NCCI */
#define ISDN_INFOCODE20_NOPLCI	0x2003	/* Out of PLCI resources */
#define ISDN_INFOCODE20_NONCCI	0x2004	/* Out of NCCI resources */
#define ISDN_INFOCODE20_NOLIS	0x2005	/* Out of LISTEN resources */
#define ISDN_INFOCODE20_NOFAX	0x2006	/* Out of FAX resources */
#define ISDN_INFOCODE20_EPARAM	0x2007	/* illegal message parameter coding */

    /* Values for class 0x30xx:	Requested services not supported errors */
#define ISDN_INFOCODE30_B1PROT	0x3001	/* B1 protocol not supported */
#define ISDN_INFOCODE30_B2PROT	0x3002	/* B2 protocol not supported */
#define ISDN_INFOCODE30_B3PROT	0x3003	/* B3 protocol not supported */
#define ISDN_INFOCODE30_B1PARAM	0x3004	/* B1 proto parameter not supported */
#define ISDN_INFOCODE30_B2PARAM	0x3005	/* B2 proto parameter not supported */
#define ISDN_INFOCODE30_B3PARAM	0x3006	/* B3 proto parameter not supported */
#define ISDN_INFOCODE30_BPROTC	0x3007	/* B proto combination not supported */
#define ISDN_INFOCODE30_NCPI	0x3008	/* NCPI not supported */
#define ISDN_INFOCODE30_CIP	0x3009	/* CIP not supported */
#define ISDN_INFOCODE30_FLAGS	0x300a	/* flags not supported */
#define ISDN_INFOCODE30_FACILTY	0x300b	/* facility not supported */
#define ISDN_INFOCODE30_DATALEN	0x300c	/* data len not supported in protocol */
#define ISDN_INFOCODE30_RESET	0x300d	/* reset not supported in proto */

/*
 * Info Element
 * This type is used by:
 *	ISDN_INFO.ISDN_IND
 */

typedef union isdnInfoEl
{
    void	*dummy;		/* ## Q.931 */
    isdnDword_t	sum;
} isdnInfoEl_t;

/*
 * Information Mask
 * This type is used by:
 *	ISDN_LISTEN.ISDN_REQ
 */

typedef isdnDword_t	isdnInfoMsk_t;

    /* Bit masks for isdnInfoMsk_t */
#define ISDN_INFOMSK_CAUSE	0x00000001	/* [0] cause */
#define ISDN_INFOMSK_DATETIME	0x00000002	/* [1] date/Time */
#define ISDN_INFOMSK_DISPLAY	0x00000004	/* [2] display */
#define ISDN_INFOMSK_USERINFO	0x00000008	/* [3] user-user info */
#define ISDN_INFOMSK_CALLPROG	0x00000010	/* [4] call progression */
#define ISDN_INFOMSK_FACILITY	0x00000020	/* [5] facility */
#define ISDN_INFOMSK_CHARGING	0x00000040	/* [6] charging */

/*
 * Information Number
 * This type is used by:
 *	ISDN_INFO.ISDN_IND
 */

typedef isdnWord_t	isdnInfoNbr_t;

    /* Bit values for isdnInfoNbr_t */
#define ISDN_INFONBRMSK_ELEMPTY	0x8000	/* [15] Info element empty */
#define ISDN_INFONBRMSK_SUPINFO	0x4000	/* [14] supplementary info */

/*
 * Low Layer Compatibility
 * This type is used by:
 *	ISDN_CONNECT.ISDN_REQ
 *	ISDN_CONNECT.ISDN_IND
 *	ISDN_CONNECT.ISDN_RESP
 *	ISDN_CONNECT_ACTIVE.ISDN_IND
 */

typedef struct isdnLLC
{
    void	*dummy;		/* ## Q.931 */
} isdnLLC_t;

/*
 * Manufacturers ID
 * This type is used by:
 *	ISDN_MANUFACTURER.ISDN_REQ
 *	ISDN_MANUFACTURER.ISDN_CONF
 *	ISDN_MANUFACTURER.ISDN_IND
 *	ISDN_MANUFACTURER.ISDN_RESP
 */

typedef isdnDword_t	isdnManuID_t;

/*
 * Network Control Connection Identifier
 * This type is used by:
 *	ISDN_CONNECT_B3.ISDN_IND
 *	ISDN_CONNECT_B3.ISDN_RESP
 *	ISDN_CONNECT_B3_ACTIVE.ISDN_IND
 *	ISDN_CONNECT_B3_ACTIVE.ISDN_RESP
 *	ISDN_CONNECT_B3_T90_ACTIVE.ISDN_IND
 *	ISDN_CONNECT_B3_T90_ACTIVE.ISDN_RESP
 *	ISDN_DISCONNECT_B3.ISDN_REQ
 *	ISDN_DISCONNECT_B3.ISDN_CONF
 *	ISDN_DISCONNECT_B3.ISDN_IND
 *	ISDN_DISCONNECT_B3.ISDN_RESP
 *	ISDN_DATA_B3.ISDN_REQ
 *	ISDN_DATA_B3.ISDN_CONF
 *	ISDN_DATA_B3.ISDN_IND
 *	ISDN_DATA_B3.ISDN_RESP
 *	ISDN_RESET_B3.ISDN_REQ
 *	ISDN_RESET_B3.ISDN_CONF
 *	ISDN_RESET_B3.ISDN_IND
 *	ISDN_RESET_B3.ISDN_RESP
 */

typedef isdnDword_t     isdnNCCI_t;

    /* Bit fields for isdnNCCI_t */
#define ISDN_NCCIMSK_NCCI	0xffff0000	/* NCCI */
#define ISDN_NCCIMSK_PLCI	0x0000ff00	/* PLCI */
#define ISDN_NCCIMSK_EXT	0x00000080	/* 1=External, 0=Internal */
#define ISDN_NCCIMSK_CTLR	0x0000007f	/* Controller */

/*
 * Network Control Protocol Information
 * This type is used by:
 *	ISDN_CONNECT_B3.ISDN_REQ
 *	ISDN_CONNECT_B3.ISDN_IND
 *	ISDN_CONNECT_B3.ISDN_RESP
 *	ISDN_CONNECT_B3_ACTIVE.ISDN_IND
 *	ISDN_CONNECT_B3_T90_ACTIVE.ISDN_IND
 *	ISDN_DISCONNECT_B3.ISDN_REQ
 *	ISDN_DISCONNECT_B3.ISDN_IND
 *	ISDN_RESET_B3.ISDN_REQ
 *	ISDN_RESET_B3.ISDN_IND
 */

#define	MAX_PACKET_BYTES	16	/* may be enough */

typedef struct isdnNCPI
{
    struct
    {
	isdnByte_t	conf;
	isdnByte_t	chanGroup;
	isdnByte_t	chanNumb;
	isdnByte_t	packet_bytes[MAX_PACKET_BYTES]; /* X.25 PLP packet */
    } P123;						/* bytes after type */
    struct
    {
	isdnWord_t	rate;
	isdnWord_t	resolution;
	isdnWord_t	format;
	isdnWord_t	pages;
	struct
	{
		void	*dummy;
	} recvdID;	/* ID of remote side */
    } FAX;
} isdnNCPI_t;

/*
 * Physical Link Connection Identification
 * This type is used by:
 *	ISDN_CONNECT.ISDN_CONF
 *	ISDN_CONNECT.ISDN_IND
 *	ISDN_CONNECT.ISDN_RESP
 *	ISDN_CONNECT_ACTIVE.ISDN_IND
 *	ISDN_CONNECT_ACTIVE.ISDN_RESP
 *	ISDN_DISCONNECT.ISDN_REQ
 *	ISDN_DISCONNECT.ISDN_CONF
 *	ISDN_DISCONNECT.ISDN_IND
 *	ISDN_DISCONNECT.ISDN_RESP
 *	ISDN_ALERT.ISDN_REQ
 *	ISDN_ALERT.ISDN_CONF
 *	ISDN_INFO.ISDN_REQ
 *	ISDN_INFO.ISDN_CONF
 *	ISDN_INFO.ISDN_IND
 *	ISDN_INFO.ISDN_RESP
 *	ISDN_CONNECT_B3.ISDN_REQ
 *	ISDN_CONNECT_B3.ISDN_CONF
 *	ISDN_SELECT_B_PROTOCOL.ISDN_REQ
 *	ISDN_SELECT_B_PROTOCOL.ISDN_CONF
 */

typedef isdnDword_t	isdnPLCI_t;

    /* Bit fields for isdnPLCI_t */
#define ISDN_PLCIMSK_PLCI	0x0000ff00	/* PLCI */
#define ISDN_PLCIMSK_EXT	0x00000080	/* 1=External, 0=Internal */
#define ISDN_PLCIMSK_CTLR	0x0000007f	/* Controller */

/*
 * Disconnect reason codes
 * This type is used by:
 *	ISDN_DISCONNECT_B3.ISDN_IND
 */

typedef isdnWord_t	isdnReason_t;

    /* Values for isdnReason_t */
#define ISDN_REASON_OK		0x0000	/* Normal clearing, no cause */
#define ISDN_REASON_L1PERR	0x3301	/* protocol error layer 1 */
#define ISDN_REASON_L2PERR	0x3302	/* protocol error layer 2 */
#define ISDN_REASON_L3PERR	0x3303	/* protocol error layer 3 */
#define ISDN_REASON_APPANS	0x3304	/* another application got that call */
#define ISDN_REASON_NET		0x3400	/* Disconnect caused by network */
#define ISDN_REASON_NETMASK	0x00ff	/* Cause info element from network */

/*
 * Disconnect_B3 reason codes
 * This type is used by:
 *	ISDN_DISCONNECT_B3.ISDN_IND
 */

typedef isdnWord_t	isdnReason_B3_t;

    /* Values for isdnReason_t */
#define ISDN_REASONB3_OK   	0x0000	/* clearing according to protocol */
#define ISDN_REASONB3_L1PERR	0x3301	/* protocol error layer 1 */
#define ISDN_REASONB3_L2PERR	0x3302	/* protocol error layer 2 */
#define ISDN_REASONB3_L3PERR	0x3303	/* protocol error layer 3 */
					/* T.30 specific reasons */
#define ISDN_REASONB3_CONNECT	0x3311	/* connect not sucessful */
#define ISDN_REASONB3_TRAINING	0x3312	/* no connect, training error */
#define ISDN_REASONB3_PRETRANS	0x3313	/* xfer mode supported be remote */
#define ISDN_REASONB3_RABORT	0x3314	/* remote abort */
#define ISDN_REASONB3_RPROCERR	0x3315	/* remote procedure error */
#define ISDN_REASONB3_LUNDERRUN	0x3316	/* local tx data underrun */
#define ISDN_REASONB3_LOVERFLOW 0x3317	/* local rx data overflow */
#define ISDN_REASONB3_LABORT	0x3318	/* local abort */
#define ISDN_REASONB3_PARAM	0x3319	/* illegal parameter coding */

/*
 * Reject
 * This type is used by:
 *	ISDN_CONNECT.ISDN_RESP
 *	ISDN_CONNECT_B3.ISDN_RESP
 */

typedef isdnWord_t	isdnReject_t;

    /* Values for isdnReject_t */
#define ISDN_REJECT_OK		0	/* Accept the call */
#define ISDN_REJECT_IGNORE	1	/* Ignore the call */
#define ISDN_REJECT_NORMAL	2	/* Reject, normal call clearing */
#define ISDN_REJECT_BUSY	3	/* Reject, user busy */
#define ISDN_REJECT_NOCIRCUIT	4	/* Reject, available circuit/channel */
#define ISDN_REJECT_FACILITY	5	/* Reject, facility rejected */
#define ISDN_REJECT_CHANNEL	6	/* Reject, channel unacceptable */
#define ISDN_REJECT_DESTINATION	7	/* Reject, incompatible destination */
#define ISDN_REJECT_OUTOFORDER	8	/* Reject, destination out of order */

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *									 *
 *		Command messages concerning signalling protocol		 *
 *									 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * ISDN_CONNECT
 *
 * ISDN_CONNECT.ISDN_REQ	Initiates an outgoing physical connection
 * ISDN_CONNECT.ISDN_CONF	Local confirmation of request
 * ISDN_CONNECT.ISDN_IND	Indicates an incoming physical connection
 * ISDN_CONNECT.ISDN_RESP	Response to indication
 */

#define ISDN_CONNECT		0x02

typedef struct isdn_connect_req
{
    isdnCtrlr_t		ctlr;		/* Controller */
    isdnCIPvalue_t	cip;		/* Compatibility Information Profile */
    isdnCalledNbr_t	calledNbr;	/* Called party number */
    isdnCallingNbr_t	callingNbr;	/* Calling party number */
    isdnCalledAdr_t	calledAddr;	/* Called party subaddress */
    isdnCallingAdr_t	callingAddr;	/* Calling party subaddress */
    isdnBproto_t	bProto;		/* B protocol to be used */
    isdnBC_t		bc;		/* Bearer Capability */
    isdnLLC_t		llc;		/* Low Layer Compatibility */
    isdnHLC_t		hlc;		/* High Layer Compatibility */
    isdnAddInfo_t	info;		/* Additional info elements */
} isdn_connect_req_t;

typedef struct isdn_connect_conf
{
    isdnPLCI_t		plci;		/* Physical Link Connection Ident */
    isdnInfo_t		info;		/* Information status code */
} isdn_connect_conf_t;

typedef struct isdn_connect_ind
{
    isdnPLCI_t		plci;		/* Physical Link Connection Ident */
    isdnCIPvalue_t	cip;		/* Compatibility Information Profile */
    isdnCalledNbr_t	calledNbr;	/* Called party number */
    isdnCallingNbr_t	callingNbr;	/* Calling party number */
    isdnCalledAdr_t	calledAddr;	/* Called party subaddress */
    isdnCallingAdr_t	callingAddr;	/* Calling party subaddress */
    isdnBC_t		bc;		/* Bearer Capability */
    isdnLLC_t		llc;		/* Low Layer Compatibility */
    isdnHLC_t		hlc;		/* High Layer Compatibility */
    isdnAddInfo_t	info;		/* Additional info elements */
} isdn_connect_ind_t;

typedef struct isdn_connect_resp
{
    isdnPLCI_t		plci;		/* Physical Link Connection Ident */
    isdnReject_t	reject;		/* Reject */
    isdnBproto_t	bProto;		/* B protocol to be used */
    isdnConnNbr_t	ConnNbr;	/* Connected number */
    isdnConnAdr_t	ConnAddr;	/* Connected subaddress */
    isdnLLC_t		llc;		/* Low Layer Compatibility */
    isdnAddInfo_t	info;		/* Additional info elements */
} isdn_connect_resp_t;

/*
 * ISDN_CONNECT_ACTIVE
 *
 * ISDN_CONNECT_ACTIVE.ISDN_IND  Indicates the activation of a physical conn
 * ISDN_CONNECT_ACTIVE.ISDN_RESP Response to indication
 */

#define ISDN_CONNECT_ACTIVE	0x03

UNUSED_MSG_TYPE(isdn_connect_active_req_t);
UNUSED_MSG_TYPE(isdn_connect_active_conf_t);

typedef struct isdn_connect_active_ind
{
    isdnPLCI_t		plci;		/* Physical Link Connection Ident */
    isdnConnNbr_t	ConnNbr;	/* Connected number */
    isdnConnAdr_t	ConnAddr;	/* Connected subaddress */
    isdnLLC_t		llc;		/* Low Layer Compatibility */
} isdn_connect_active_ind_t;

typedef struct isdn_connect_active_resp
{
    isdnPLCI_t		plci;		/* Physical Link Connection Ident */
} isdn_connect_active_resp_t;

/*
 * ISDN_DISCONNECT
 *
 * ISDN_DISCONNECT.ISDN_REQ	Initiates clearing of a physical connection
 * ISDN_DISCONNECT.ISDN_CONF	Local confirmation of request
 * ISDN_DISCONNECT.ISDN_IND	Indicates the clearing of a physical connection
 * ISDN_DISCONNECT.ISDN_RESP	Response to indication
 */

#define ISDN_DISCONNECT		0x04

typedef struct isdn_disconnect_req
{
    isdnPLCI_t		plci;		/* Physical Link Connection Ident */
    isdnAddInfo_t	info;		/* Additional info elements */
} isdn_disconnect_req_t;

typedef struct isdn_disconnect_conf
{
    isdnPLCI_t		plci;		/* Physical Link Connection Ident */
    isdnInfo_t		info;		/* Information status code */
} isdn_disconnect_conf_t;

typedef struct isdn_disconnect_ind
{
    isdnPLCI_t		plci;		/* Physical Link Connection Ident */
    isdnReason_t	reason;		/* Disconnect reason codes */
} isdn_disconnect_ind_t;

typedef struct isdn_disconnect_resp
{
    isdnPLCI_t		plci;		/* Physical Link Connection Ident */
} isdn_disconnect_resp_t;

/*
 * ISDN_ALERT
 *
 * ISDN_ALERT.ISDN_REQ		Initiates compatibility to call
 * ISDN_ALERT.ISDN_CONF		Local confirmation of request
 */

#define ISDN_ALERT		0x01

typedef struct isdn_alert_req
{
    isdnPLCI_t		plci;		/* Physical Link Connection Ident */
    isdnAddInfo_t	info;		/* Additional info elements */
} isdn_alert_req_t;

typedef struct isdn_alert_conf
{
    isdnPLCI_t		plci;		/* Physical Link Connection Ident */
    isdnInfo_t		info;		/* Information status code */
} isdn_alert_conf_t;

UNUSED_MSG_TYPE(isdn_alert_ind_t);
UNUSED_MSG_TYPE(isdn_alert_resp_t);

/*
 * ISDN_INFO
 *
 * ISDN_INFO.ISDN_REQ	Initiates sending of signalling
 * ISDN_INFO.ISDN_CONF	Local confirmation of request
 * ISDN_INFO.ISDN_IND	Indicates selected signalling info
 * ISDN_INFO.ISDN_RESP	Response to indication
 */

#define ISDN_INFO		0x08

typedef struct isdn_info_req
{
    isdnPLCI_t		plci;		/* Physical Link Connection Ident */
    isdnCalledNbr_t	calledNbr;	/* Called party number */
    isdnAddInfo_t	info;		/* Additional info elements */
} isdn_info_req_t;

typedef struct isdn_info_conf
{
    isdnPLCI_t		plci;		/* Physical Link Connection Ident */
    isdnInfo_t		info;		/* Information status code */
} isdn_info_conf_t;

typedef struct isdn_info_ind
{
    isdnPLCI_t		plci;		/* Physical Link Connection Ident */
    isdnInfoNbr_t	infoNumber;	/* Information element identifier */
    isdnInfoEl_t	infoElement;	/* Information element */
} isdn_info_ind_t;

typedef struct isdn_info_resp
{
    isdnPLCI_t		plci;		/* Controller/PLCI */
} isdn_info_resp_t;

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *									 *
 *		Command messages concerning logical connections		 *
 *									 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * ISDN_CONNECT_B3
 *
 * ISDN_CONNECT_B3.ISDN_REQ	Outgoing logical connection
 * ISDN_CONNECT_B3.ISDN_CONF	Local confirmation of request
 * ISDN_CONNECT_B3.ISDN_IND	Incoming logical connection
 * ISDN_CONNECT_B3.ISDN_RESP	Response to indication
 */

#define ISDN_CONNECT_B3		0x82

typedef struct isdn_connect_b3_req
{
    isdnPLCI_t		plci;		/* Physical Link Connection Ident */
    isdnNCPI_t		ncpi;		/* Network Control Protocol Info */
} isdn_connect_b3_req_t;

typedef struct isdn_connect_b3_conf
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
    isdnInfo_t		info;		/* Information status code */
} isdn_connect_b3_conf_t;

typedef struct isdn_connect_b3_ind
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
    isdnNCPI_t		ncpi;		/* Network Control Protocol Info */
} isdn_connect_b3_ind_t;

typedef struct isdn_connect_b3_resp
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
    isdnReject_t	reject;		/* Reject */
    isdnNCPI_t		ncpi;		/* Network Control Protocol Info */
} isdn_connect_b3_resp_t;

/*
 * ISDN_CONNECT_B3_ACTIVE
 *
 * ISDN_CONNECT_B3_ACTIVE.ISDN_IND	Activation of logical connection
 * ISDN_CONNECT_B3_ACTIVE.ISDN_RESP	Response to indication
 */

#define ISDN_CONNECT_B3_ACTIVE	0x83

UNUSED_MSG_TYPE(isdn_connect_b3_active_req_t);
UNUSED_MSG_TYPE(isdn_connect_b3_active_conf_t);

typedef struct isdn_connect_b3_active_ind
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
    isdnNCPI_t		ncpi;		/* Network Control Protocol Info */
} isdn_connect_b3_active_ind_t;

typedef struct isdn_connect_b3_active_resp
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
} isdn_connect_b3_active_resp_t;

/*
 * ISDN_CONNECT_B3_T90_ACTIVE
 *
 * ISDN_CONNECT_B3_T90_ACTIVE.ISDN_IND	Switching from T.70NL to T.90NL
 * ISDN_CONNECT_B3_T90_ACTIVE.ISDN_RESP	Response to indication
 */

#define ISDN_CONNECT_B3_T90_ACTIVE	0x88

UNUSED_MSG_TYPE(isdn_connect_b3_t90_active_req_t);
UNUSED_MSG_TYPE(isdn_connect_b3_t90_active_conf_t);

typedef struct isdn_connect_b3_t90_active_ind
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
    isdnNCPI_t		ncpi;		/* Network Control Protocol Info */
} isdn_connect_b3_t90_active_ind_t;

typedef struct isdn_connect_b3_t90_active_resp
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
} isdn_connect_b3_t90_active_resp_t;

/*
 * ISDN_DISCONNECT_B3
 *
 * ISDN_DISCONNECT_B3.ISDN_REQ	Clear a logical connection
 * ISDN_DISCONNECT_B3.ISDN_CONF	Local confirmation of request
 * ISDN_DISCONNECT_B3.ISDN_IND	Logical connection clear
 * ISDN_DISCONNECT_B3.ISDN_RESP	Response to indication
 */

#define ISDN_DISCONNECT_B3	0x84

typedef struct isdn_disconnect_b3_req
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
    isdnNCPI_t		ncpi;		/* Network Control Protocol Info */
} isdn_disconnect_b3_req_t;

typedef struct isdn_disconnect_b3_conf
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
    isdnInfo_t		info;		/* Information status code */
} isdn_disconnect_b3_conf_t;

typedef struct isdn_disconnect_b3_ind
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
    isdnReason_B3_t	reason;		/* Disconnect reason codes */
    isdnNCPI_t		ncpi;		/* Network Control Protocol Info */
} isdn_disconnect_b3_ind_t;

typedef struct isdn_disconnect_b3_resp
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
} isdn_disconnect_b3_resp_t;

/*
 * ISDN_DATA_B3
 *
 * ISDN_DATA_B3.ISDN_REQ	Sending data on logical
 * ISDN_DATA_B3.ISDN_CONF	Local confirmation of request
 * ISDN_DATA_B3.ISDN_IND	Incoming data on logical conn
 * ISDN_DATA_B3.ISDN_RESP	Response to indication
 */

#define ISDN_DATA_B3		0x86

typedef struct isdn_data_b3_req
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
    isdnData_t		data;		/* Data pointer, not used */
    isdnDataLen_t	dataLen;	/* Data length */
    isdnHandle_t	dataHandle;
    isdnFlags_t		flags;
} isdn_data_b3_req_t;

typedef struct isdn_data_b3_conf
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
    isdnWord_t		dataHandle;
    isdnInfo_t		info;		/* Information status code */
} isdn_data_b3_conf_t;

typedef struct isdn_data_b3_ind
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
    isdnData_t		data;		/* Data pointer, not used */
    isdnDataLen_t	dataLen;	/* Data length */
    isdnHandle_t	dataHandle;
    isdnFlags_t		flags;
} isdn_data_b3_ind_t;

typedef struct isdn_data_b3_resp
{
    isdnNCCI_t		ncci;	/* Network Control Connection ID */
    isdnWord_t		dataHandle;
} isdn_data_b3_resp_t;

/*
 * ISDN_RESET_B3
 *
 * ISDN_RESET_B3.ISDN_REQ	Reset of a logical connection
 * ISDN_RESET_B3.ISDN_CONF	Local confirmation of request
 * ISDN_RESET_B3.ISDN_IND	Reset of a logical connection
 * ISDN_RESET_B3.ISDN_RESP	Response to indication
 */

#define ISDN_RESET_B3		0x87

typedef struct isdn_reset_b3_req
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
    isdnNCPI_t		ncpi;		/* Network Control Protocol Info */
} isdn_reset_b3_req_t;

typedef struct isdn_reset_b3_conf
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
    isdnInfo_t		info;		/* Information status code */
} isdn_reset_b3_conf_t;

typedef struct isdn_reset_b3_ind
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
    isdnNCPI_t		ncpi;		/* Network Control Protocol Info */
} isdn_reset_b3_ind_t;

typedef struct isdn_reset_b3_resp
{
    isdnNCCI_t		ncci;		/* Network Control Connection ID */
} isdn_reset_b3_resp_t;

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *									 *
 *		Administrative and other command messages		 *
 *									 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * ISDN_LISTEN
 *
 * ISDN_LISTEN.ISDN_REQ		Activates call indications
 * ISDN_LISTEN.ISDN_CONF	Local confirmation of request
 */

#define ISDN_LISTEN		0x05

typedef struct isdn_listen_req
{
    isdnCtrlr_t		ctlr;		/* Controller */
    isdnInfoMsk_t	infoMask;	/* Information mask */
    isdnCIPmask_t	cipMask;
    isdnCIPmask_t	cipMask2;
    isdnCallingNbr_t	callingNbr;	/* Calling party number */
    isdnCallingAdr_t	callingAddr;	/* Calling party subaddress */
} isdn_listen_req_t;

typedef struct isdn_listen_conf
{
    isdnCtrlr_t		ctlr;		/* Controller */
    isdnInfo_t		info;		/* Information status code */
} isdn_listen_conf_t;

UNUSED_MSG_TYPE(isdn_listen_ind_t);
UNUSED_MSG_TYPE(isdn_listen_resp_t);

/*
 * ISDN_FACILITY
 *
 * ISDN_FACILITY.ISDN_REQ	Requests additional facilities
 * ISDN_FACILITY.ISDN_CONF	Local confirmation of request
 * ISDN_FACILITY.ISDN_IND	Indicates additional facilities
 * ISDN_FACILITY.ISDN_RESP	Response to indication
 */

#define ISDN_FACILITY		0x80

typedef struct isdn_facility_req
{
    isdnCtrlr_t		ctlr;		/* Controller/PLCI/NCCI */
    isdnFacility_t	selector;	/* Facility selector */
    isdnFRP_t		frp;		/* Facility request parameter */
} isdn_facility_req_t;

typedef struct isdn_facility_conf
{
    isdnCtrlr_t		ctlr;		/* Controller/PLCI/NCCI */
    isdnInfo_t		info;		/* Information status code */
    isdnFacility_t	selector;	/* Facility selector */
    isdnFCP_t		fcp;		/* Facility confirmation parameter */
} isdn_facility_conf_t;

typedef struct isdn_facility_ind
{
    isdnCtrlr_t		ctlr;		/* Controller/PLCI/NCCI */
    isdnFacility_t	selector;	/* Facility selector */
    isdnFIP_t		fip;		/* Facility indication parameter */
} isdn_facility_ind_t;

typedef struct isdn_facility_resp
{
    isdnCtrlr_t		ctlr;		/* Controller/PLCI/NCCI */
    isdnFacility_t	selector;	/* Facility selector */
} isdn_facility_resp_t;

/*
 * ISDN_SELECT_B_PROTOCOL
 *
 * ISDN_SELECT_B_PROTOCOL.ISDN_REQ	Selects current protocol stack
 * ISDN_SELECT_B_PROTOCOL.ISDN_CONF	Local confirmation of request
 */

#define ISDN_SELECT_B_PROTOCOL	0x41

typedef struct isdn_select_b_protocol_req
{
    isdnPLCI_t		plci;		/* Physical Link Connection Ident */
    isdnBproto_t	bProto;		/* B protocol to be used */
} isdn_select_b_protocol_req_t;

typedef struct isdn_select_b_protocol_conf
{
    isdnPLCI_t		plci;		/* Physical Link Connection Ident */
    isdnInfo_t		info;		/* Information status code */
} isdn_select_b_protocol_conf_t;

UNUSED_MSG_TYPE(isdn_select_b_protocol_ind_t);
UNUSED_MSG_TYPE(isdn_select_b_protocol_resp_t);

/*
 * ISDN_MANUFACTURER
 *
 * ISDN_MANUFACTURER.ISDN_REQ	Manufacturer specific operation
 * ISDN_MANUFACTURER.ISDN_CONF	Manufacturer specific operation
 * ISDN_MANUFACTURER.ISDN_IND	Manufacturer specific operation
 * ISDN_MANUFACTURER.ISDN_RESP	Manufacturer specific operation
 *
 * The structure passed in the data portion of these messages is
 * manufacturer implementation specific.  The first element of
 * each, however is a isdn_manufacturer_hdr_t.
 */

#define ISDN_MANUFACTURER	0xFF	/* Manufacturer specific operation */

typedef struct isdn_manufacturer_hdr
{
    isdnCtrlr_t		ctlr;		/* Controller */
    isdnManuID_t	manuID;		/* Manufacturers Identification */
} isdn_manufacturer_hdr_t;

#ifndef HAS_ISDN_MFGR_REQ
    typedef struct isdn_manufacturer_req
    {
	isdn_manufacturer_hdr_t	Head;
	isdnU8_t		data[1];
    } isdn_manufacturer_req_t;
#endif

#ifndef HAS_ISDN_MFGR_CONF
    typedef struct isdn_manufacturer_conf
    {
	isdn_manufacturer_hdr_t	Head;
	isdnU8_t		data[1];
    } isdn_manufacturer_conf_t;
#endif

#ifndef HAS_ISDN_MFGR_IND
    typedef struct isdn_manufacturer_ind
    {
	isdn_manufacturer_hdr_t	Head;
	isdnU8_t		data[1];
    } isdn_manufacturer_ind_t;
#endif

#ifndef HAS_ISDN_MFGR_RESP
    typedef struct isdn_manufacturer_resp
    {
	isdn_manufacturer_hdr_t	Head;
	isdnU8_t		data[1];
    } isdn_manufacturer_resp_t;
#endif

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Protocol information structure declarations
 *
 * These are pre-declared pinfo structures that can be used
 * in the dials(3N) routine to make a call of a particular type.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

typedef struct isdn_pinfo {
isdnNCCI_t	NCCI;		/* NCCI, connection identifier */
isdnCIPvalue_t	CIPvalue;	/* Compatibility Information Profile */
isdnByte_t	protocol_info[64]; /* protocol info variable length array */
} isdn_pinfo_t;

#define	PINFO_ISDN_SYNC	\
	/* Unrestricted digital information, HDLC framing */\
{ 0, /* NCCI, connection identifier, returned by dialer */\
\
  2, /* Compatibility Information Profile, determines Bearer Capatibility */\
     /* and sometimes High Layer Compatibility in Q.931 SETUP message     */\
\
  9, /* size of B Protocol structure */\
  0, /* B1 Protocol, 0 = 64 kBits with HDLC framing */\
  0,\
  1, /* B2 Protocol, 1 = transparent                */\
  0,\
  0, /* B3 Protocol, 0 = transparent                */\
  0,\
  0, /* B1 Configuration structure size, 0, not used */\
  0, /* B2 Configuration structure size, 0, not used */\
  0, /* B3 Configuration structure size, 0, not used */\
\
  0, /* Q.931 SETUP message Bearer Capability, 0, not used */\
  0, /* Q.931 SETUP message Low Layer Capability, 0, not used */\
  0, /* Q.931 SETUP message High Layer Compatibility, 0, not used */\
  0  /* Additional information elements, 0, none */\
}\

#endif	/* _IO_ND_SYS_SCOISDN_H */
