#ident	"@(#)kern-pdi:io/hba/fdeb/fdeb.h	1.1"
/*
 * fdsb.h       Copyright Future Domain Corp., 1993-94. All rights reserved.
 */
typedef unsigned char  osd_uchar;
typedef unsigned short osd_ushort;
typedef unsigned long  osd_ulong;

#define FDC_MAX_ADAPTERS  4
#define FDC_MAX_INQ_BYTES 36

typedef struct fdc_idev {
    osd_uchar       fdd_inq_data[FDC_MAX_INQ_BYTES + 1];
    osd_uchar       fdd_len;            /* Length required for a match */
    osd_ushort      fdd_flags;          /* "Special instruction" flags */
    osd_ushort      fdx_head_count;     /* Handshake byte count of block hd */
    osd_ushort      fdx_tail_count;     /* Handshake byte count of block tl */
} fdc_idev_t;
