#ifndef _IO_ASCII_H	/* wrapper symbol for kernel use */
#define _IO_ASCII_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/ascii.h	1.4"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#define A_NUL	0
#define A_SOH	1
#define A_STX	2
#define A_ETX	3
#define A_EOT	4
#define A_ENQ	5
#define A_ACK	6
#define A_BEL	7
#define A_BS	8
#define A_HT	9
#define A_NL	10
#define A_LF	10
#define A_VT	11
#define A_FF	12
#define A_NP	12
#define A_CR	13
#define A_SO	14
#define A_SI	15
#define A_DLE	16
#define A_DC1	17
#define A_DC2	18
#define A_DC3	19
#define A_DC4	20
#define A_NAK	21
#define A_SYN	22
#define A_ETB	23
#define A_CAN	24
#define A_EM	25
#define A_SUB	26
#define A_ESC	27
#define A_FS	28
#define A_GS	29
#define A_RS	30
#define A_US	31
#define A_DEL	127
#define A_CSI	0x9b

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_ASCII_H */
