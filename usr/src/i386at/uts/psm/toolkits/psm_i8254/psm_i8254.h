#ifndef _PSM_PSM_I8254_H	/* wrapper symbol for kernel use */
#define _PSM_PSM_I8254_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:psm/toolkits/psm_i8254/psm_i8254.h	1.1.3.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_PSM)
#if _PSM != 2
#error "Unsupported PSM version"
#endif
#else
#error "Header file not valid for Core OS"
#endif

#ifndef __PSM_SYMREF

struct __PSM_dummy_st {
	int *__dummy1__;
	const struct __PSM_dummy_st *__dummy2__;
};
#define __PSM_SYMREF(symbol) \
	extern int symbol; \
	static const struct __PSM_dummy_st __dummy_PSM = \
		{ &symbol, &__dummy_PSM }

#if !defined(_PSM)
__PSM_SYMREF(No_PSM_version_defined);
#pragma weak No_PSM_version_defined
#else
#define __PSM_ver(x) _PSM_##x
#define _PSM_ver(x) __PSM_ver(x)
__PSM_SYMREF(_PSM_ver(_PSM));
#endif

#endif  /* __PSM_SYMREF */


#include <svc/psm.h>



/*
 *         INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *     This software is supplied under the terms of a license 
 *    agreement or nondisclosure agreement with Intel Corpo-
 *    ration and may not be copied or disclosed except in
 *    accordance with the terms of that agreement.
 */

/*
 * Definitions for 8254 Programmable Interrupt Timer ports on AT 386 
 */
#define	I8254_CTR0_PORT	0x40		/* counter 0 port */	
#define	I8254_CTR1_PORT	0x41		/* counter 1 port */	
#define	I8254_CTR2_PORT	0x42		/* counter 2 port */	
#define	I8254_CTL	0x03		/* Mask to OR with a port to get the control port */
#define	I8254_AUX_PORT	0x61		/* PIT auxiliary port */


#define I8254_CLKHZ	1193167		/* Clock frequency in Hz */

/*
 * Definitions for 8254 commands 
 */

#define	I8254_READMODE		0x30	/* load least significant byte followed
					 * by most significant byte */
#define I8254_LATCH		0xC2	/* latch status & counter value */
#define I8254_NDIVMODE		0x04	/* divide by N counter */
#define	I8254_SQUAREMODE	0x06	/* square-wave mode */
#define	I8254_ENDSIGMODE	0x00	/* assert OUT at end-of-count mode*/

/*
 * Definitions of bits in the status word
 */
#define I8254_OUT_HIGH	0x80		/* OUT pin */

/*
 * Structure for describing an i8254 counter.
 */
typedef struct {
        ms_lockp_t      lockp;          /* lock for serializing access */
        ms_port_t       port;           /* port number for counter chip */
        unsigned int    clkval;		/* value last loaded into counter */
} i8254_params_t;

/*
 * Function prototypes for toolkit procedures.
 */
void            i8254_init(i8254_params_t *, ms_port_t, ms_time_t);
unsigned int    i8254_get_time(i8254_params_t *);
void            i8254_deinit(i8254_params_t *);


#if defined(__cplusplus)
	}
#endif

#endif /* _PSM_PSM_I8254_H */
