#ifndef _IO_DMA_H	/* wrapper symbol for kernel use */
#define	_IO_DMA_H	/* subject to change without notice */

#ident	"@(#)dma.h	1.10"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*      Copyright (c) 1988, 1989 Intel Corp.            */
/*        All Rights Reserved   */
/*
 *      INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *      This software is supplied under the terms of a license
 *      agreement or nondisclosure agreement with Intel Corpo-
 *      ration and may not be copied or disclosed except in
 *      accordance with the terms of that agreement.
 */

#ifdef _KERNEL_HEADERS

#include <io/i8237A.h>		/* PORTABILITY */
#include <mem/kmem.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/i8237A.h>		/* PORTABILITY */
#include <sys/kmem.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


#if defined(_KERNEL) || defined(_KMEMUSER)

/* the DMA Status Structure */
struct dma_stat {
        paddr_t         targaddr;      /* physical address of buffer */
        paddr_t         targaddr_hi;   /* more for 64-bit addresses */
        paddr_t         reqraddr;      /* physical address of buffer */
        paddr_t         reqraddr_hi;   /* more for 64-bit addresses */
        unsigned short  count;         /* size of bytes in buffer */
        unsigned short  count_hi;      /* more for big blocks */
};

/* the DMA Buffer Descriptor structure */
struct dma_buf {
        unsigned short  reserved;    /* alignment pad */
        unsigned short   count;      /* size of block */
        paddr_t   address;	     /* phys addr of data block */
        paddr_t   physical;	     /* phys addr of next dma_buf */
        struct dma_buf  *next_buf;   /* next buffer descriptor */
        unsigned short  reserved_hi; /* alignment pad */
        unsigned short  count_hi;    /* for big blocks */
        unsigned long   address_hi;  /* for 64-bit addressing */
        unsigned long   physical_hi; /* for 64-bit addressing */
};

/* the DMA Command Block structure */
struct dma_cb {
        struct dma_cb  *next;       /* free list link */
        struct dma_buf *targbufs;   /* list of target data buffers */
        struct dma_buf *reqrbufs;   /* list of requestor data buffers */
        unsigned char  command;     /* Read/Write/Translate/Verify */
        unsigned char  targ_type;   /* Memory/IO */
        unsigned char  reqr_type;   /* Memory/IO */
        unsigned char  targ_step;   /* Inc/Dec/Hold */
        unsigned char  reqr_step;   /* Inc/Dec/Hold */
        unsigned char  trans_type;  /* Single/Demand/Block/Cascade */
        unsigned char  targ_path;   /* 8/16/32 */
        unsigned char  reqr_path;   /* 8/16/32 */
        unsigned char  cycles;      /* 1 or 2 */
        unsigned char  bufprocess;  /* Single/Chain/Auto-Init */
        unsigned short dummy;           /* alignment pad */
        char           *procparms;  /* parameter buffer for appl call */
        int            (*proc)();   /* address of application call routine */
};

#define DMA_CMD_READ    0x0
#define DMA_CMD_WRITE   0x1
#define DMA_CMD_TRAN    0x2
#define DMA_CMD_VRFY    0x3

#define DMA_TYPE_MEM    0x0
#define DMA_TYPE_IO     0x1

#define DMA_STEP_INC    0x0
#define DMA_STEP_DEC    0x1
#define DMA_STEP_HOLD   0x2

#define DMA_TRANS_SNGL  0x0
#define DMA_TRANS_DMND  0x1
#define DMA_TRANS_BLCK  0x2
#define DMA_TRANS_CSCD  0x3

#define DMA_PATH_8      0x0
#define DMA_PATH_16     0x1
#define DMA_PATH_32     0x2
#define DMA_PATH_64     0x3

/*
 * We could use DMA_PATH_64 to mean this but why not
 * just put in a separate define.
 */
#define DMA_PATH_16B    0x4     /* 16-bit path but byte count */

#define DMA_CYCLES_1    0x0
#define DMA_CYCLES_2    0x1

/*
 * For the EISA bus we will use the following definitions for DMA_CYCLES
 *    DMA_CYCLES_1 = Compatible timing
 *    DMA_CYCLES_2 = Type "A" timing
 *    DMA_CYCLES_3 = Type "B" timing
 *    DMA_CYCLES_4 = Burst timing
 */
#define DMA_CYCLES_3    0x2
#define DMA_CYCLES_4    0x3

#define DMA_BUF_SNGL    0x0
#define DMA_BUF_CHAIN   0x1
#define DMA_BUF_AUTO    0x2

#define DMA_SLEEP       0x0
#define DMA_NOSLEEP     0x1

#define DMA_ENABLE	0x2	/* enable channel */
#define DMA_DISABLE	0x4	/* disable channel */

/* some common defined constants */
#ifndef PDMA
#define PDMA 5
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

/* public function routines */
#if defined(__STDC__)
extern void            dma_init(void);
extern void            dma_intr(int chan);
extern int             dma_prog(struct dma_cb *dmacbp, int chan, uchar_t mode);
extern boolean_t       dma_cascade(int chan, uchar_t mode);
extern uchar_t         dma_get_best_mode(struct dma_cb *dmacbp);
extern int             dma_swsetup(struct dma_cb *dmacbp, int chan,
				   uchar_t mode);
extern void            dma_swstart(struct dma_cb *dmacbp, int chan,
				   uchar_t mode);
extern void            dma_stop(int chan);
extern void            dma_enable(int chan);
extern void            dma_disable(int chan);
extern struct dma_cb  *dma_get_cb(uchar_t mode);
extern void            dma_free_cb(struct dma_cb *dmacbp);
extern struct dma_buf *dma_get_buf(uchar_t mode);
extern void            dma_free_buf(struct dma_buf *dmabufp);
extern void	       dma_get_chan_stat(struct dma_stat *, int chan);
extern void	       dma_physreq(int chan, int datapath, physreq_t *preqp);
#else
extern void            dma_init();
extern void            dma_intr();
extern int             dma_prog();
extern boolean_t       dma_cascade();
extern uchar_t         dma_get_best_mode();
extern int             dma_swsetup();
extern void            dma_swstart();
extern void            dma_stop();
extern void            dma_enable();
extern void            dma_disable();
extern struct dma_cb  *dma_get_cb();
extern void            dma_free_cb();
extern struct dma_buf *dma_get_buf();
extern void            dma_free_buf();
extern void	       dma_get_chan_stat();
extern void	       dma_physreq();
#endif

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_DMA_H */
