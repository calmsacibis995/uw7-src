#ident	"@(#)kern-pdi:io/layer/mpio/ioctl_ext.h	1.1.3.1"

#ifndef	_IO_LAYER_MP_IOCTL_H
#define _IO_LAYER_MP_IOCTL_H

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This module contains the multi-path (MP) device control commands
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>  /* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * devctl arguments for MP kernel ioctls
 */

#define MP_IOCTL_MULTIPORT_INFO			(SDI_DEVICE_GMPINFO)
#define MP_IOCTL_TRESPASS				(SDI_DEVICE_TRESPASS)
#define SD_PATH_REPAIRED  				(SDI_DEVICE_RPAIRPTH)

#define MP_PORT_SIGNATURE_LEN			64
/*
 * Per port multi-ported information packet. This data structure
 * is used as part of the MP_IOCTL_MULTIPORT_INFO command
 */
typedef struct multi_port_info_packet {
	uchar_t		signature_valid;	/* set if 'signature' field is valid	*/
	uchar_t		active;				/* set if this port is active			*/
	char		signature [MP_PORT_SIGNATURE_LEN];

} mp_info_t, * mp_info_p_t;

/*
 * Multi-ported (MP) definitions - clariion specific for now
 */

#define	MAX_NUMBER_OF_PORTS		0x2	/* max # of ports MP device can have	*/

/*
 * One of the arguments of the MP ioctl(2)
 */
typedef struct get_multi_port_info_packet {
	uint        lun;				/* the lun number of this port			*/
	uint		number_of_ports;	/* 1 if single port, 2 if dual-ported	*/
	mp_info_t 	info[MAX_NUMBER_OF_PORTS];

} get_mp_info_t, * get_mp_info_p_t;


#if defined(__cplusplus)
    }
#endif

#endif /* _IO_LAYER_MP_IOCTL_H */
